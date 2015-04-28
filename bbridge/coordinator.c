#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>


struct sensor_info_s{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint16_t temp;
    uint32_t pressure;
    float humidity;
    float luminosity;
};





int main(int argc, char **argv)
{
    pthread_t thread_array[NUMBER_OF_THREADS];

    for(int i=0; i < NUMBER_OF_THREADS; i++){
        pthread_create( &thread_array[i], NULL, thread_functions[i], (void *)tread_data[i]);
    }

    fprintf(stderr, "going into while loop in main\n");

    while(1){

    } // while(1)

} // End of int main(int argc, char **argv)

    
