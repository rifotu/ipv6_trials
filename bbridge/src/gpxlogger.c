/*
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "gps.h"
#include "gpsd_config.h"
#include "gpsdclient.h"
#include "revision.h"

#include "gpxlogger.h"

static char *progname;
static struct fixsource_t source;

/**************************************************************************
 *
 * Transport-layer-independent functions
 *
 **************************************************************************/
const double zeros_13 = 10000000000.0;
const double zeros_4 = 10000.0;

static struct gps_data_t gpsdata;
static FILE *logfile;
static bool intrack = false;
static time_t timeout = 5;	/* seconds */
static double minmove = 0;	/* meters */

// Global Variables  RT
static double old_lat_G = 0.0;
static double old_lon_G = 0.0;
static pthread_t threadID_G; 

// Private function prototypes  RT
static void debug_dump(struct gps_data_t *gpsdata);
static void conditionally_log_fix(struct gps_data_t *gpsdata);
static void quit_handler(int signum);
static int init_gpsd(void);
static void* get_gpsd(void *arg);


static void debug_dump(struct gps_data_t *gpsdata)
{

    if(gpsdata->fix.longitude!=gpsdata->fix.longitude || gpsdata->fix.altitude!=gpsdata->fix.altitude){
        fprintf(stderr, "No fix for now\n");
    }else{
        fprintf(stderr,"Longitude: %lf\nLatitude: %lf\nAltitude: %lf\nAccuracy: %lf\n\n",
                gpsdata->fix.latitude, gpsdata->fix.longitude, gpsdata->fix.altitude,
                (gpsdata->fix.epx>gpsdata->fix.epy)?gpsdata->fix.epx:gpsdata->fix.epy);
    }
}

long long get_height(void)
{
    return 35;
}


long long get_longitude(void)
{
    double lon = 0.0;

    lon = old_lon_G * zeros_13;
    //printf("lon: %llu\n", (long long)lon);
    return (long long)lon;
}

long long get_latitude(void)
{
    double lat = 0.0;

    //TODO  mutex these against the thread
    lat = old_lat_G * zeros_13;
    //printf("lat: %llu\n", (long long)lat);
    return (long long)lat;
}

static void conditionally_log_fix(struct gps_data_t *gpsdata)
{
    static double int_time, old_int_time;
    //static double old_lat_G, old_lon_G;
    static bool first = true;

    //debug_dump(gpsdata);
	//old_lat_G = gpsdata->fix.latitude;
	//old_lon_G = gpsdata->fix.longitude;

    int_time = gpsdata->fix.time;
    //if ((int_time == old_int_time) || gpsdata->fix.mode < MODE_2D)
	//return;

    /* may not be worth logging if we've moved only a very short distance */
    if (minmove>0 && !first && earth_distance(
					gpsdata->fix.latitude,
					gpsdata->fix.longitude,
					old_lat_G, old_lon_G) < minmove)
	return;

    /*
     * Make new track if the jump in time is above
     * timeout.  Handle jumps both forward and
     * backwards in time.  The clock sometimes jumps
     * backward when gpsd is submitting junk on the
     * dbus.
     */
    if (fabs(int_time - old_int_time) > timeout && !first) {
	//print_gpx_trk_end();
	intrack = false;
    }

    if (!intrack) {
	//print_gpx_trk_start();
	intrack = true;
	if (first)
	    first = false;
    }

    old_int_time = int_time;
    if (minmove >= 0) {
	old_lat_G = gpsdata->fix.latitude;
	old_lon_G = gpsdata->fix.longitude;
    }
    sleep(2);
}

static void quit_handler(int signum)
{
    /* don't clutter the logs on Ctrl-C */
    if (signum != SIGINT)
	syslog(LOG_INFO, "exiting, signal %d received", signum);
    //print_gpx_footer();
    (void)gps_close(&gpsdata);
    exit(EXIT_SUCCESS);
}

/**************************************************************************
 *
 * Main sequence
 *
 **************************************************************************/
static int init_gpsd(void)
{
    struct exportmethod_t *method = NULL;

    method = export_default();
    logfile = stdout;


    if (method->magic != NULL) {
	source.server = (char *)method->magic;
	source.port = NULL;
	source.device = NULL;
    } else {
	source.server = (char *)"localhost";
	source.port = (char *)"3001";
	source.device = NULL;
    }

    printf("hello\n");
	gpsd_source_spec(NULL, &source);

    /* catch all interesting signals */
    (void)signal(SIGTERM, quit_handler);
    (void)signal(SIGQUIT, quit_handler);
    (void)signal(SIGINT, quit_handler);

    return 0;
}

static void* get_gpsd(void *arg)
{
    unsigned int flags = WATCH_ENABLE;
    //if (gps_open(source.server, source.port, &gpsdata) != 0) {
    if (gps_open("localhost", "3001", &gpsdata) != 0) {
	(void)fprintf(stderr,
		      "%s: no gpsd running or network error: %d, %s\n",
		      progname, errno, gps_errstr(errno));
	exit(EXIT_FAILURE);
    }

    if (source.device != NULL)
	flags |= WATCH_DEVICE;
    (void)gps_stream(&gpsdata, flags, source.device);

    //print_gpx_header();
    (void)gps_mainloop(&gpsdata, 5000000, conditionally_log_fix);
    //print_gpx_footer();
    (void)gps_close(&gpsdata);

    exit(EXIT_SUCCESS);
}

void init_gps_stuff(void)
{
    init_gpsd();
    pthread_create(&threadID_G, NULL, get_gpsd, NULL);
}

void dismiss_gps_stuff(void)
{
    //TODO
    // stop the pthread
    // stop the gpsd stuff
}

    

//int main(void)
//{
//    pthread_t idThreads[1];
//
//    init_gpsd();
//    pthread_create( &idThreads[0], NULL, get_gpsd, NULL);
//
//    while(1){
//
//        sleep(1);
//    }
//    return 0;
//}
