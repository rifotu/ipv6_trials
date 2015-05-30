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

static char *progname;
static struct fixsource_t source;

/**************************************************************************
 *
 * Transport-layer-independent functions
 *
 **************************************************************************/

static struct gps_data_t gpsdata;
static FILE *logfile;
static bool intrack = false;
static time_t timeout = 5;	/* seconds */
static double minmove = 0;	/* meters */
#ifdef CLIENTDEBUG_ENABLE
static int debug;
#endif /* CLIENTDEBUG_ENABLE */


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


static void print_fix(struct gps_data_t *gpsdata, double time)
{
    char tbuf[CLIENT_DATE_MAX+1];

    (void)fprintf(logfile,"   <trkpt lat=\"%f\" lon=\"%f\">\n",
		 gpsdata->fix.latitude, gpsdata->fix.longitude);
    if ((isnan(gpsdata->fix.altitude) == 0))
	(void)fprintf(logfile,"    <ele>%f</ele>\n", gpsdata->fix.altitude);
    (void)fprintf(logfile,"    <time>%s</time>\n",
		 unix_to_iso8601(time, tbuf, sizeof(tbuf)));
    switch (gpsdata->fix.mode) {
    case MODE_3D:
	(void)fprintf(logfile,"    <fix>3d</fix>\n");
	break;
    case MODE_2D:
	(void)fprintf(logfile,"    <fix>2d</fix>\n");
	break;
    case MODE_NO_FIX:
	(void)fprintf(logfile,"    <fix>none</fix>\n");
	break;
    default:
	/* don't print anything if no fix indicator */
	break;
    }

    if ((gpsdata->fix.mode > MODE_NO_FIX) && (gpsdata->satellites_used > 0))
	(void)fprintf(logfile,"    <sat>%d</sat>\n", gpsdata->satellites_used);
    if (isnan(gpsdata->dop.hdop) == 0)
	(void)fprintf(logfile,"    <hdop>%.1f</hdop>\n", gpsdata->dop.hdop);
    if (isnan(gpsdata->dop.vdop) == 0)
	(void)fprintf(logfile,"    <vdop>%.1f</vdop>\n", gpsdata->dop.vdop);
    if (isnan(gpsdata->dop.pdop) == 0)
	(void)fprintf(logfile,"    <pdop>%.1f</pdop>\n", gpsdata->dop.pdop);

    (void)fprintf(logfile,"   </trkpt>\n");
    (void)fflush(logfile);
}


static void conditionally_log_fix(struct gps_data_t *gpsdata)
{
    static double int_time, old_int_time;
    static double old_lat, old_lon;
    static bool first = true;

    debug_dump(gpsdata);

    int_time = gpsdata->fix.time;
    if ((int_time == old_int_time) || gpsdata->fix.mode < MODE_2D)
        
	return;

    /* may not be worth logging if we've moved only a very short distance */
    if (minmove>0 && !first && earth_distance(
					gpsdata->fix.latitude,
					gpsdata->fix.longitude,
					old_lat, old_lon) < minmove)
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
    if (minmove > 0) {
	old_lat = gpsdata->fix.latitude;
	old_lon = gpsdata->fix.longitude;
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

static void usage(void)
{
    fprintf(stderr,
	    "Usage: %s [-V] [-h] [-d] [-i timeout] [-f filename] [-m minmove]\n"
	    "\t[-e exportmethod] [server[:port:[device]]]\n\n"
	    "defaults to '%s -i 5 -e %s localhost:2947'\n",
	    progname, progname, export_default()->name);
    exit(EXIT_FAILURE);
}


int init_gpsd(void)
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
}

void* get_gpsd(void *arg)
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


int main(void)
{
    pthread_t idThreads[1];

    init_gpsd();
    pthread_create( &idThreads[0], NULL, get_gpsd, NULL);
    //get_gpsd();

    while(1){

        sleep(1);
    }

    return 0;
}
