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


// IPv6 communication with the sensor nodes
void * get_data_from_sensor(char *ip_addr, int port)
{
    struct sockaddr_in6 serv_addr;
    int sockfd;
    int slen = sizeof(serv_addr);
    char dummy_msg[4] = "get";

    struct sockaddr_in6 resp_addr; // source address of echo
    int resp_addr_size = sizeof(resp_addr); // In - out of address size for recvfrom()
    int resp_buf_len; // length of received response
    void *resp_buf;

    if( -1 == ( resp_buf_len = recvfrom(sockfd, buf, resp_ 

    // socket() creates an endpoint for communication and returns a descriptor.
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) <= 0){
        fprintf(stderr, "socket");
    }

    // sets the first n bytes of the area starting at s to zero
    bzero( &serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(port);

    // convert IPv4 and IPv6 addresses from text to binary form 
    if( 0 == inet_pton(AF_INET6, ip_addr, &serv_addr.sin6_addr)){
        fprintf(stderr, "inet_aton() failed\n");
        //TODO:  exit safely from the pthread
    }

    if( -1 == sendto(sockfd, dummy_msg, strlen(dummy_msh), 0, (struct sockaddr *) &serv_addr, slen) ){
        err("sendto() function");
    }
    



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

    
