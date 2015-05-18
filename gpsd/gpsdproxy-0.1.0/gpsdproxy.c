/*
 Copyright (C) 2009-2011 Niccolo Rigacci

 This file is part of GPSDproxy.

 GPSDproxy is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 GPSDproxy is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

 Author        Niccolo Rigacci <niccolo@rigacci.org>
 Version       0.1.0    2011-04-05

 Open a TCP/IP connection to a gpsd process, read position at
 specified intervals and proxy position data to a remote host via
 an (unreliable) connection using an UDP socket.
*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <netdb.h>
#include <gps.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#undef COMMUNICATE_NMEA                 // Use float instead of NMEA crappy values.

#define DEFAULT_ID "gpsdproxy"          // Default ID of running client instance.
#define DEFAULT_GPSD_HOST "localhost"   // Default gpsd host to read from.
#define DEFAULT_REMOTE_PORT "2948"      // Default remote UDP port to communicate to.
#define DEFAULT_INTERVAL 10             // Interval between position communication (int sec).

#define LOG_TAG "gpsdproxy"             // Prefix for log entries.
#define DEFAULT_DEBUG LOG_INFO          // Suppress syslog(3) entries less important than.
#define MAX_UDP_DATAGRAM 1400           // Max size of UDP packet sent to remote.
#define MAX_LOG_FORMAT_STR 200          // Max size of log string.
#define MAX_ID_LEN 20                   // Max size of identifier string.
#define GPSD_ERROR_HOLDOFF 60           // Pause before reopening gpsd socket, in case of error.
#define REMOTE_SOCKET_EXPIRE 120        // Reopen remote socket every that seconds.

//------------------------------------------------------------------------
// Gloabl declarations and variables.
//------------------------------------------------------------------------
void mylog(int priority, char *format, ...);
void sig_handler(int sig);
int connectsock(const char *host, const char *service, const char *transport);
void communicate_position(char *buffer);
static void handle_raw_gps_data(struct gps_data_t *gpsdata, char *buf UNUSED, size_t len UNUSED);

// From gpsdclient.h:
struct fixsource_t {
    char *spec;
    char *server;
    char *port;
    char *device;
};

int debug = DEFAULT_DEBUG;              // Ignores syslog(3) entries above this value.
int be_daemon = 0;                      // Run in background if daemon.
int killed;                             // Run untill signal received.

const char *remote_host = NULL;         // Hostname or address of remote host.
const char *remote_port = DEFAULT_REMOTE_PORT;
const char *id = DEFAULT_ID;            // Our identifier to the remote server.
time_t interval = DEFAULT_INTERVAL;     // Position communication interval.
struct fixsource_t source;              // Our GPS source: gpsd connection, device name, etc.

//------------------------------------------------------------------------
// Main loop.
//------------------------------------------------------------------------
int main(int argc, char *argv[]) {

    const char *pid_file    = NULL;

    int data, gpsd_error, opt;
    struct gps_data_t gpsdata;
    struct timeval tv;
    FILE *fp;
    pid_t pid;
    fd_set fds;

    source.server = DEFAULT_GPSD_HOST;  // Source gpsd hostname or address.
    source.port   = DEFAULT_GPSD_PORT;  // Source gpsd port name or number.
    source.device = NULL;               // Set to NULL to read from any GPS device.

    /* Parse command line options */
    while ((opt = getopt(argc, argv, "bd:f:h:i:p:r:s:t:")) != -1) {
        switch (opt) {
        case 'b':
            be_daemon = 1;
            break;
        case 'f':
            pid_file = optarg;
            break;
        case 'h':
            remote_host = optarg;
            break;
        case 'p':
            remote_port = optarg;
            break;
        case 's':
            source.server = optarg;
            break;
        case 'r':
            source.port = optarg;
            break;
        case 'i':
            id = optarg;
            break;
        case 't':
            interval = atoi(optarg);
            break;
        case 'd':
            debug = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s -h host [-p port] [-i id] [-t interval] [-s gpsd_host] [-r gpsd_port] [-d debug] [-f pid_file] [-b]\n", LOG_TAG);
            exit(1);
        }
    }

    if (remote_host == NULL) {
        fprintf(stderr, "%s: error: Remote host (option -h) is required\n", LOG_TAG);
        exit(1);
    }

    if (strlen(id) > MAX_ID_LEN) {
        fprintf(stderr, "%s: error: Id string too long\n", LOG_TAG);
        exit(1);
    }

    if (strstr(id, " ") != NULL || strstr(id, "*") != NULL || strstr(id, ",") != NULL) {
        fprintf(stderr, "%s: error: Illegal characters in id string\n", LOG_TAG);
        exit(1);
    }

    /* Fork into background */
    if (be_daemon) {
        if (daemon(0, 0) == -1) {
            perror("Could not fork() into background");
            exit(1);
        }
        /* Prepare syslog logging */
        openlog(LOG_TAG, LOG_PID, LOG_USER);
    }

    /* Write PID file */
    if (pid_file != NULL) {
        if ((fp = fopen(pid_file, "w")) == NULL) {
            mylog(LOG_ERR, "cannot write PID file.");
        } else {
            fprintf(fp, "%i\n", getpid());
            fclose(fp);
        }
    }

    killed = 0;
    signal(SIGHUP, sig_handler);
    signal(SIGTERM, sig_handler);
    gpsd_error = 1;

    while (!killed) {

        // Open a tcp socket to gpsd.
        while (gpsd_error) {
            mylog(LOG_NOTICE, "opening gpsd connection to %s:%s", source.server, source.port);
            if (gps_open_r(source.server, source.port, &gpsdata) != 0) {
                mylog(LOG_ERR, "connection to gpsd failed: gps_open_r(): %s", strerror(errno));
                mylog(LOG_INFO, "waiting %d seconds", GPSD_ERROR_HOLDOFF);
                sleep(GPSD_ERROR_HOLDOFF);
            } else {
                mylog(LOG_INFO, "socket to gpsd opened");
                // Set a function handler for raw data.
                gps_set_raw_hook(&gpsdata, handle_raw_gps_data);
                // Ask gpsd to stream data to us. 
                mylog(LOG_NOTICE, "enable GPS data watching");
                if (gps_stream(&gpsdata, WATCH_ENABLE, NULL) == -1) {
                    mylog(LOG_ERR, "enable watch: gps_stream() error: %s", strerror(errno));
                } else {
                    gpsd_error = 0;
                }
            }
        }

        // Wait until some data is available.
        FD_ZERO(&fds);
        FD_SET(gpsdata.gps_fd, &fds);
        tv.tv_usec = 0;
        tv.tv_sec = 60;
        data = select(gpsdata.gps_fd + 1, &fds, NULL, NULL, &tv);

        // Read the data, this will trigger the handler which does the actual job.
        if (data == -1) {
            mylog(LOG_ERR, "waiting GPS data: select() error: %s", strerror(errno));
        } else if (data == 0) {
            mylog(LOG_DEBUG, "waiting GPS data: select() timeout");
        } else {
            if (gps_read(&gpsdata) == -1) {
                mylog(LOG_ERR, "reading GPS data: gps_read() error");
                gpsd_error = 1;
            }
        }

        if (gpsd_error) {
            // There was some error with gpsd.
            mylog(LOG_NOTICE, "closing gpsd socket and waiting %d seconds", GPSD_ERROR_HOLDOFF);
            (void)gps_close(&gpsdata);
            sleep(GPSD_ERROR_HOLDOFF);
        }

    }

    mylog(LOG_NOTICE, "closing gpsd connection");
    (void)gps_close(&gpsdata);
    unlink(pid_file);
    exit(0);
}

//------------------------------------------------------------------------
// Print a log message to syslog or stdout.
//------------------------------------------------------------------------
void mylog(int priority, char *format, ...) {

    va_list args;
    time_t t;
    struct tm *tm;
    char timestamp[20];
    char my_format[MAX_LOG_FORMAT_STR];

    if (priority > debug) return;

    va_start(args, format);
    if (be_daemon) {
        /* If running as a daemon, write to syslog */
        vsyslog(priority, format, args);
    } else {
        /* Write to stdout */
        t = time(NULL);
        tm = localtime(&t);
        if (tm != NULL)
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);
        else
            timestamp[0] = '\0';
        snprintf(my_format, sizeof(my_format), "%s %s: %d %s", timestamp, LOG_TAG, priority, format);
        vfprintf(stdout, my_format, args);
        fprintf(stdout, "\n");
    }
    va_end(args);
}

//------------------------------------------------------------------------
// Get signal(s) and raise a termination flag.
//------------------------------------------------------------------------
void sig_handler(int sig) {
    killed = 1;
}

//------------------------------------------------------------------------
// Connect to an internet host using a socket.
//------------------------------------------------------------------------
int connectsock(const char *host, const char *service, const char *transport) {

    struct hostent      *phe;   /* pointer to host information entry     */
    struct servent      *pse;   /* pointer to service information entry  */
    struct protoent     *ppe;   /* pointer to protocol information entry */
    struct sockaddr_in  sin;    /* an Internet endpoint address          */
    int s, type;                /* socket descriptor and socket type     */
    int i;

    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;

    /* Check if service is expressed as port number or service name */
    for (i = 0; service[i] != '\0'; i++)
        if (service[i] < '0' || service[i] > '9')
            break;
    if (i >= strlen(service)) {
        /* Convert port number into network byte order */
        sin.sin_port = htons(atoi(service));
    } else {
        /* Map service name to port number */
        if (pse = getservbyname(service, transport))
            sin.sin_port = pse->s_port;
        else if ((sin.sin_port = htons((u_short)atoi(service))) == 0) {
            mylog(LOG_ERR, "can't get \"%s\" service entry", service);
            return -1;
        }
    }

    /* Map host name to IP address, allowing for dotted decimal */
    if (phe = gethostbyname(host))
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        mylog(LOG_ERR, "can't get \"%s\" host entry", host);
        return -1;
    }

    /* Map transport protocol name to protocol number */
    if ((ppe = getprotobyname(transport)) == 0) {
        mylog(LOG_ERR, "can't get \"%s\" protocol entry", transport);
        return -1;
    }

    /* Use protocol to choose a socket type */
    if (strcmp(transport, "udp") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;

    /* Allocate a socket */
    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0) {
       mylog(LOG_ERR, "can't create socket: %s", strerror(errno));
       return -1;
    }

    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
       mylog(LOG_ERR, "can't connect to %s.%s: %s", host, service, strerror(errno));
       return -1;
    }

    return s;
}

//------------------------------------------------------------------------
// Open an UDP socket and send a buffer of data.
// Socket is keept open across all function invocation; it will be
// reopened in case of error and on every REMOTE_SOCKET_EXPIRE timeout.
//------------------------------------------------------------------------
void communicate_position(char *buffer) {

    static int remote_socket = -1;
    static int remote_error = 1;
    static time_t remote_socket_time;
    size_t count;

    // Remote connection is considered unreliable,
    // do not emit logs lower than LOG_WARNING.

    // Close the remote socket if it timed-out.
    if (! remote_error && (time(NULL) - remote_socket_time) > REMOTE_SOCKET_EXPIRE) {
        mylog(LOG_INFO, "closing expired remote socket");
        close(remote_socket);
        remote_error = 1;
    }

    // Open an udp socket to the remote host.
    if (remote_error) {
        mylog(LOG_INFO, "connecting remote udp socket to %s:%s", remote_host, remote_port);
        remote_socket = connectsock(remote_host, remote_port, "udp");
        if (remote_socket == -1) {
            mylog(LOG_WARNING, "remote socket connection failed");
        } else {
            mylog(LOG_INFO, "remote udp socket opened");
            remote_socket_time = time(NULL);
            remote_error = 0;
        }
    }

    // Send data via UDP packet.
    if (! remote_error) {
        mylog(LOG_INFO, "send udp packet: %s", buffer);
        count = write(remote_socket, buffer, strlen(buffer));
        if (count == -1) {
            mylog(LOG_WARNING, "remote socket write() failed");
            remote_error = 1;
        } else if (count != strlen(buffer)) {
            mylog(LOG_WARNING, "incomplete write() to remote socket");
            remote_error = 1;
        }
        if (remote_error) {
            mylog(LOG_WARNING, "closing remote udp socket");
            close(remote_socket);
        }
    }
}

//------------------------------------------------------------------------
// This function is called whenever some data is read from GPS.
//------------------------------------------------------------------------
static void handle_raw_gps_data(struct gps_data_t *gpsdata,
    char *buf UNUSED, size_t len UNUSED) {

    int i;
    char buffer[MAX_UDP_DATAGRAM];
    char sum[4];
    unsigned char checksum;
    struct gps_fix_t *gpsfix;
    static time_t last_update_time = 0;

    //mylog(LOG_DEBUG, "GPS data handler: handle_raw_gps_data()");

    // Communicate position every interval seconds.
    if ((time(NULL) - last_update_time) < interval) return;

    // We need at least a fix with timestamp and lat/lon.
    if ((gpsdata->status == STATUS_NO_FIX)
        || !(gpsdata->set & TIME_SET)
        || !(gpsdata->set & LATLON_SET)) return;

    // Accept data only from requested device.
    if (gpsdata->dev.path[0] != '\0'
        && source.device != NULL
        && strcmp(gpsdata->dev.path, source.device) != 0) return;

    //mylog(LOG_DEBUG, "got valid data from device %s", gpsdata->dev.path);
    //mylog(LOG_DEBUG, "data: dev.path=%s status=%d satellites_used=%d fix.time=%f fix.latitude=%07.4f set=%d",
    //    gpsdata->dev.path, gpsdata->status, gpsdata->satellites_used,
    //    gpsdata->fix.time, gpsdata->fix.latitude, gpsdata->set);

    gpsfix = &gpsdata->fix;

#ifdef COMMUNICATE_NMEA
    // Communicate a pseudo NMEA string to the remote host.
    // NMEA timestamp is something like "HHMMSS.SSS".
    time_t gpstime;
    char nmea_time[10];
    gpstime = (time_t)gpsfix->time; // Cast double to time_t (integer)
    strftime(nmea_time, 7, "%H%M%S", gmtime(&gpstime));
    snprintf(nmea_time + 6, 4, ".%02d", (int)((gpsfix->time - gpstime) * 100));
    // NMEA altitute is something like "m.m,M".
    char alt[10]; snprintf(alt, 10, "%.1f,M", gpsfix->altitude);
    snprintf(buffer, sizeof(buffer),
        "$GPGGA,%s,%02d%07.4f,%s,%03d%07.4f,%s,%d,%02d,%.2f,%s,,,,",
        (gpsdata->set & TIME_SET)       ? nmea_time : "",
        abs((int)gpsfix->latitude),
        fabs(gpsfix->latitude - (int)gpsfix->latitude) * 60,
        (gpsfix->latitude >= 0) ? "N" : "S",
        abs((int)gpsfix->longitude),
        fabs(gpsfix->longitude - (int)gpsfix->longitude) * 60,
        (gpsfix->longitude >= 0) ? "E" : "W",
        gpsdata->status,
        (gpsdata->set & SATELLITE_SET) ? gpsdata->satellites_used : 0,
        (!isnan(gpsdata->dop.hdop))    ? gpsdata->dop.hdop : 99.99,
        (gpsdata->set & ALTITUDE_SET)  ? alt : ",");
#else
    // Communicate a string of decimal numbers to the remote host.
    snprintf(buffer, sizeof(buffer),
        "%s,%.2f,%.9f,%.9f,%.2f,%.2f,%.2f,%d,%02d,%.1f,",
        id,
        gpsfix->time,
        gpsfix->latitude,
        gpsfix->longitude,
        (gpsdata->set & ALTITUDE_SET)  ? gpsfix->altitude : NAN,
        (gpsdata->set & SPEED_SET)     ? gpsfix->speed : NAN,
        (gpsdata->set & TRACK_SET)     ? gpsfix->track : NAN,
        gpsdata->status,
        (gpsdata->set & SATELLITE_SET) ? gpsdata->satellites_used : 0,
        (!isnan(gpsdata->dop.hdop))    ? gpsdata->dop.hdop : 99.99);
#endif /* NMEA_OUTPUT */

    // Append the checksum (if there is sufficient space).
    if ((sizeof(buffer) - strlen(buffer)) >= sizeof(sum)) {
        checksum = '\0';
        for(i = 1; buffer[i] != '\0'; i++) checksum ^= buffer[i];
        snprintf(sum, sizeof(sum), "*%02X", (unsigned)checksum);
        strcat(buffer, sum);
    }

    mylog(LOG_DEBUG, "sending %s", buffer);
    communicate_position(buffer);
    last_update_time = time(NULL);
}
