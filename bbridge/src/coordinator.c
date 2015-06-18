#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "lcd.h"
#include "cloud.h"
#include "gpxlogger.h"
#include "sensorNode.h"

int main(int argc, char **argv)
{
    void *wsn = NULL;
    void *wsn_flat_cloud = NULL;
    void *wsn_flat_lcd = NULL;
    int size = 0;

    sleep(5);
    init_gps_stuff();
    initiate_connection_2_cloud();
    initiate_wsn();
    initiate_lcd_comm();

    while(1)
    {


        // returns a pointer of struct wsn_data_s
        //printf("calling prep a set of wsn data\n");
        printf("................\n");
        wsn = prep_a_set_of_wsn_data();

        // record a certain number of wsn data set
        // in case you may need it in the future
        // Don't worry about freeing stuff, as it
        // will be handled later in dismiss_wsn function
        record_a_set_of_wsn_data(wsn);
        
        // returns a buffer containing all data in flat format
        wsn_flat_cloud = unroll_wsn_data(wsn);
        size = calc_size_for_unrolled_data(wsn);
        print_cloud_data(wsn_flat_cloud);
        send_data_2_cloud((uint8_t *)wsn_flat_cloud, size);
        //sendme.j = get_latitude();
        //sendme.k = get_longitude();
        //send_data_2_cloud((uint8_t *)&sendme, sizeof(struct test_s));

        wsn_flat_lcd = unroll_wsn_data_lcd(wsn);
        size = calc_size_for_unrolled_lcd_data(wsn);
        send_data_2_lcd((uint8_t *)wsn_flat_lcd, size);
        
        free(wsn_flat_cloud);
        free(wsn_flat_lcd);
        sleep(10);
    }

    dismiss_lcd_comm();
    dismiss_wsn();
    kill_connection_2_cloud();

    return 0;

} // End of int main(int argc, char **argv)

    
