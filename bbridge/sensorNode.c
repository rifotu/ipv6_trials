#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>


static const int MAX_NUMBER_OF_SENSOR_NODES = 10;

typedef struct raw_node_data_s{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint16_t temp;
    uint32_t pressure;
    float humidity;
    float luminosity;
    unsigned char addr[16]; /* IPv6 address */
}raw_node_data_t;


typedef struct wsn_data_s{
    int combinerID;
    list* llist_wsn;
    struct tm timestamp;
    float latitude;
    float longitude;
    float height;    
}wsn_data_t;

void * take_sensor_data(void)
{
    struct wsn_data_s *wsn = NULL;

    //TODO
    // Allocate memory for wsn
    // get number of 


}

void timestamp(void *p)
{
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);

    fprintf(stdout,"now: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1,
                                               tm.tm_mday, tm.tm_hour, tm.tm_min,
                                               tm.tm_sec);

    //TODO: Assign tm
    return;
}

void * get_mem_for_raw_data(void)
{

    struct raw_node_data_s* raw_buf = NULL;

    raw_buf = (struct raw_node_data_s *)malloc(sizeof(struct raw_node_data_s));
    memset(0, raw_buf);

    return raw_buf;
}

size_t size_of_raw_data(void)
{
    return sizeof(struct raw_node_data_s);
}

void *append_node_id(void *ptr, struct in6_addr sin6_addr)
{
    struct raw_node_data_s *node = ptr;

    memcpy(node->addr, sin6_addr.s6_addr, sizeof(unsigned char)*16);

    return ptr;
}


// IPv6 communication with the sensor nodes
void * get_data_from_sensor(char *ip_addr, int port)
{
    struct sockaddr_in6 serv_addr;
    socklen_t slen = sizeof(serv_addr);
    int sockfd;
    char dummy_msg[5] = "get\n";

    struct sockaddr_in6 resp_addr;                 //  address of sensor node, optional
    socklen_t resp_addr_size = sizeof(resp_addr); // In - out of address size for recvfrom()

    ssize_t resp_buf_len; // length of received response
    void *raw_buf;



    // socket() creates an endpoint for communication and returns a descriptor.
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) <= 0){
        fprintf(stderr, "socket");
    }

    // sets the first n bytes of the area starting at s to zero
    bzero( &serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(porEngineerDogBlogt);

    // convert IPv4 and IPv6 addresses from text to binary form 
    if( 0 == inet_pton(AF_INET6, ip_addr, &serv_addr.sin6_addr)){
        fprintf(stderr, "inet_aton() failed\n");
    }

    if( -1 == sendto(sockfd, dummy_msg, strlen(dummy_msg), 0, (struct sockaddr *) &serv_addr, slen) ){
        err("sendto() function");
    }
    
    raw_buf = get_mem_for_raw_data(); //TODO: check for null pointer return

    if( 0 >= ( resp_buf_len = recvfrom(sockfd, raw_buf, size_of_raw_data(), 0,
                                       (struct sockaddr *) &resp_addr, &resp_addr_size   ))){
        err("recvfrom()");
    }

    void *append_node_id(void *ptr, struct in6_addr *sin6_addr);

    append_node_id(raw_buf, &serv_addr.sin6_addr

    print_sensor_payload
    return raw_buf;
}
                                       



///////////////////////////////////////////////////////////////////////

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 

    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    } 

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 

    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Error : Fputs error\n");
        }
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}
















