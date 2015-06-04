#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>

#include "list.h"

// Definitions
#define PORT_NO  51717
#define SERVER_IP "178.79.179.186"
//#define SERVER_IP "127.0.0.1"
// Global variables
int sockfd_G = 0;

static const float zeros_13 = 10000000000.0;
static const float zeros_4 = 10000.0;

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

typedef struct time_llh_s{
    int combinerID;
    int number_of_nodes;
    struct tm timestamp;
    long long latitude;
    long long longitude;
    long long height;
}time_llh_t;

typedef struct wsn_data_s{
    struct time_llh_s time_llh;
    list* llist_wsn;         // holds raw data for each sensor in the network 
}wsn_data_t;


// Private function prototypes
static long long gen_coordinate(void);
//static int gen_sensor_data(void);
static float gen_float_random(void);
static int get_payload_size(uint8_t *buf);
static void *prep_node_data(void);
static void *prep_time_llh(void);
static uint8_t * prep_uart_data(void);
static void print_uart_data(uint8_t *buf);
static void error(const char *msg);
static int initiate_connection_2_cloud(void);
static int kill_connection_2_cloud(void);
static int send_data_2_cloud(void *buf, int size);


static long long gen_coordinate(void)
{
    double a = 5.0;
    double ll = 0.0;

    ll = (((double)rand()/(double)(RAND_MAX)) * a);
    //printf("gen coordinate double: %f\n", ll);
    ll *= zeros_13;
    //printf("gen coordinate double: %f\n", ll);
    //printf("gen coordinate int: %lld\n", (long long)ll );
    return  (long long )ll;
}

//static int gen_sensor_data(void)
//{
//    float a = 5.0;
//    float ll = 0.0;
//    ll = (((float)rand()/(float)(RAND_MAX)) * a);
//    //printf("gen coordinate double: %f\n", ll);
//    ll *= zeros_4;
//    //printf("gen coordinate double: %f\n", ll);
//    //printf("gen coordinate float: %d\n", (int)ll );
//    return  (int)ll;
//}

static float gen_float_random(void)
{
    float a = 5.0;
    return (((float)rand()/(float)(RAND_MAX)) * a);
}

static void *prep_node_data(void)
{
    struct raw_node_data_s *raw = NULL;

    raw = (struct raw_node_data_s *)malloc(sizeof(struct raw_node_data_s));

    raw->accel_x =  rand() % 10 - 5;
    raw->accel_y =  rand() % 20 - 10;
    raw->accel_z =  rand() % 15 - 7;
    raw->temp = rand() % 30;
    raw->pressure = rand() % 1000;
    raw->humidity = gen_float_random();
    raw->luminosity = gen_float_random();
    memcpy(raw->addr, "hello world", strlen("hello world"));

    printf("test h: %f\n", raw->humidity); 
    printf("test l: %f\n", raw->luminosity); 
    printf("test a_x: %d\n", raw->accel_x); 
    return raw;
}

static void *prep_time_llh(void)
{
    struct time_llh_s *time_llh = NULL;
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);

    time_llh = (struct time_llh_s *)malloc(sizeof(struct time_llh_s));

    time_llh->timestamp = tm;

    //time_llh->combinerID = rand() % 5;
    time_llh->combinerID = 3;
    //time_llh->number_of_nodes = rand() % 3 + 1;
    time_llh->number_of_nodes = 2;
    time_llh->latitude = gen_coordinate();
    time_llh->longitude = gen_coordinate();

    return time_llh;
}

uint8_t * prep_uart_data(void)
{
    struct time_llh_s *time_llh = NULL;
    struct raw_node_data_s *raw = NULL;
    uint8_t *buf = NULL;
    int size = 0;
    int offset = 0;
    
    time_llh = prep_time_llh();

    size = sizeof(struct time_llh_s) + time_llh->number_of_nodes * sizeof(struct raw_node_data_s);
    buf = (uint8_t *)malloc(sizeof(uint8_t) * size);
    memcpy(buf, time_llh, sizeof(struct time_llh_s));

    for(int i=0; i < time_llh->number_of_nodes; i++){
        offset = sizeof(struct time_llh_s) + i * sizeof(struct raw_node_data_s);
        raw = prep_node_data();
        printf("test h: %f\n", raw->humidity); 
        printf("test l: %f\n", raw->luminosity); 
        printf("test a_x: %d\n", raw->accel_x); 
        memcpy(buf + offset, raw, sizeof(struct raw_node_data_s));
        free(raw);
    }
    
    free(time_llh);
    return buf;
}

int get_payload_size(uint8_t *buf)
{
    struct time_llh_s *time_llh = (struct time_llh_s *)buf;
    int len = sizeof(struct time_llh_s) + sizeof(struct raw_node_data_s) * time_llh->number_of_nodes;
    printf("buffer length: %d\n", len);

    return len;
}

void print_uart_data(uint8_t *buf)
{
    struct time_llh_s *time_llh = NULL;
    struct raw_node_data_s *raw = NULL;
    int offset = 0;

    time_llh = (struct time_llh_s *)buf;

    printf("combiner id: %d\n", time_llh->combinerID);
    printf("number of nodes. %d\n", time_llh->number_of_nodes);
    printf("latitude: %lld\n", time_llh->latitude);
    printf("longitude: %lld\n", time_llh->longitude);

    for(int i=0; i < time_llh->number_of_nodes; i++)
    {
        offset = sizeof(struct time_llh_s) + i * sizeof(struct raw_node_data_s);
        raw = (struct raw_node_data_s *)(buf + offset);
        //printf("P:%+6ld\n", raw->pressure);
        printf("P:%zu\n", raw->pressure);
        printf("T2:%+3d\n", raw->temp);
        printf("X:%+6d | Y:%+6d | Z:%+6d\n",  raw->accel_x,    \
                                              raw->accel_y,    \
                                              raw->accel_z );
        printf("H: %f\n", raw->humidity);
        printf("L: %f\n", raw->luminosity);
    }
    
}
    
void error(const char *msg)
{
    perror(msg);
    exit(0);
}


int initiate_connection_2_cloud(void)
{
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = PORT_NO;
    sockfd_G = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd_G < 0){
        error("ERROR opening socket");
    }

    server = gethostbyname(SERVER_IP);
    if( NULL == server ){
        fprintf(stderr, "ERROR, no such host\n");
        return -1;
    }

    bzero( (char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //bcopy((char *)server->h_addr, 
    //     (char *)&serv_addr.sin_addr.s_addr,
    //     server->h_length);
    memcpy( (char *) &serv_addr.sin_addr.s_addr,
            (char *) server->h_addr,
            server->h_length);

    serv_addr.sin_port = htons(portno);

    if( connect(sockfd_G, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR connecting");
        return -1;
    }
    return 0;
}

static int kill_connection_2_cloud(void)
{
    printf("killing connection to cloud\n");
    close(sockfd_G);
    return 0;
}


static int send_data_2_cloud(void *buf, int size)
{
    int n = 0;
    printf("sending data to cloud\n");
    n = write(sockfd_G, buf, size);
    if( n < 0){
        error("error writing to socket");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int len = 0;
    uint8_t *buffer = NULL;
 
    //printf("enter a message\n");
    //bzero(buffer,256);
    //fgets(buffer,255,stdin);

    initiate_connection_2_cloud();

    while(1)
    {
        printf("hello testing\n");
        buffer = prep_uart_data();
        len = get_payload_size(buffer);
        print_uart_data(buffer);

        //send_data_2_cloud(buffer, sizeof(buffer));
        send_data_2_cloud(buffer, len);
        sleep(5);

    }

    kill_connection_2_cloud();

    return 0;
}
 
//   int sockfd, portno, n;
 //   struct sockaddr_in serv_addr;
 //   struct hostent *server;

 //   char buffer[256];
 //   if (argc < 3) {
 //      fprintf(stderr,"usage %s hostname port\n", argv[0]);
 //      exit(0);
 //   }
 //   portno = atoi(argv[2]);
 //   sockfd = socket(AF_INET, SOCK_STREAM, 0);
 //   if (sockfd < 0) 
 //       error("ERROR opening socket");
 //   server = gethostbyname(argv[1]);
 //   if (server == NULL) {
 //       fprintf(stderr,"ERROR, no such host\n");
 //       exit(0);
 //   }
 //   bzero((char *) &serv_addr, sizeof(serv_addr));
 //   serv_addr.sin_family = AF_INET;
 //   bcopy((char *)server->h_addr, 
 //        (char *)&serv_addr.sin_addr.s_addr,
 //        server->h_length);
 //   serv_addr.sin_port = htons(portno);
 //   if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
 //       error("ERROR connecting");
 //   printf("Please enter the message: ");
 //   bzero(buffer,256);
 //   fgets(buffer,255,stdin);
 //   n = write(sockfd,buffer,strlen(buffer));
 //   if (n < 0) 
 //        error("ERROR writing to socket");
 //   bzero(buffer,256);
 //   n = read(sockfd,buffer,255);
 //   if (n < 0) 
 //        error("ERROR reading from socket");
 //   printf("%s\n",buffer);
 //   close(sockfd);
 //   return 0;
