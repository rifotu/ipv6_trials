#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "gps.h"
#include "gpdsclient.h"

#define DEFAULT_GPSD_HOST   "localhost"
#define DEFAULT_PORT        "3001"

// private global variables
static struct gps_data_t gpsdata;

int getGPSData(gps_data_t *gpsdata);
void debugDump(gps_data_t *gpsdata);



int main(int argc, char **argv)
{
    
    printf("starting test program\n");
    if( gps_open(DEFAULT_GPSD_HOST, DEFAULT_PORT, &gpsdata) != 0){
        printf("problem\n");
        return -1;
    }
    
    printf("connected");
    return 0;
}

void debugDump(gps_data_t *gpsdata){
    fprintf(stderr,"Longitude: %lf\nLatitude: %lf\nAltitude: %lf\nAccuracy: %lf\n\n",
                gpsdata->fix.latitude, gpsdata->fix.longitude, gpsdata->fix.altitude,
                (gpsdata->fix.epx>gpsdata->fix.epy)?gpsdata->fix.epx:gpsdata->fix.epy);
}

int getGPSData(gps_data_t *gpsdata){
    //connect to GPSd
    if(gps_open("localhost", "3001", gpsdata)<0){
        fprintf(stderr,"Could not connect to GPSd\n");
        return(-1);
    }
 
    //register for updates
    gps_stream(gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);
     
    fprintf(stderr,"Waiting for gps lock.");
    //when status is >0, you have data.
    while(gpsdata->status==0){
        //block for up to .5 seconds
        if (gps_waiting(gpsdata, 500)){
            //dunno when this would happen but its an error
            if(gps_read(gpsdata)==-1){
                fprintf(stderr,"GPSd Error\n");
                gps_stream(gpsdata, WATCH_DISABLE, NULL);
                gps_close(gpsdata);
                return(-1);
                break;
            }
            else{
                //status>0 means you have data
                if(gpsdata->status>0){
                    //sometimes if your GPS doesnt have a fix, it sends you data anyways
                    //the values for the fix are NaN. this is a clever way to check for NaN.
                    if(gpsdata->fix.longitude!=gpsdata->fix.longitude || gpsdata->fix.altitude!=gpsdata->fix.altitude){
                        fprintf(stderr,"Could not get a GPS fix.\n");
                        gps_stream(gpsdata, WATCH_DISABLE, NULL);
                        gps_close(gpsdata);
                        return(-1);
                    }
                    //otherwise you have a legitimate fix!
                    else
                        fprintf(stderr,"\n");
                }
                //if you don't have any data yet, keep waiting for it.
                else
                    fprintf(stderr,".");
            }
        }
        //apparently gps_stream disables itself after a few seconds.. in this case, gps_waiting returns false.
        //we want to re-register for updates and keep looping! we dont have a fix yet.
        else
            gps_stream(gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);
 
        //just a sleep for good measure.
        sleep(1);
    }
    //cleanup
    gps_stream(gpsdata, WATCH_DISABLE, NULL);
    gps_close(gpsdata);
}
