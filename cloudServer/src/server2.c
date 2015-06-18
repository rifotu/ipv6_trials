/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <math.h>

#include "list.h"

// Definitions
#define PORT_NO  51717

// Constant Global Variables
const int maxPayloadSize = 512;
const char *rawDataFile = "rawData.sql";
const char *geoLocationFile = "geoLocation.sql";

// Global Variables
struct config_s config_G;


// Structure definitions
typedef struct config_s{
    FILE *fp_raw;
    FILE *fp_geo;
    list *llist_dataset;
}config_t;

typedef struct raw_node_data_s{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint16_t temp;
    uint32_t pressure;
    float humidity;
    float luminosity;
    char addr[25]; /* IPv6 address */
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
static void error(const char *msg);
static void free_ll_raw_data(void *data);
static void free_ll_wsn_history(void *data);
static void print_sensor_payload(struct wsn_data_s * wsn);

// Public function prototypes
int init_stuff(void);
int dismiss_stuff(void);
int prep_raw_data_file(struct wsn_data_s *wsn);
int add_rolled_wsn_data_to_ll(struct wsn_data_s *wsn);
int get_ll_size(void);
struct wsn_data_s * roll_wsn_data(void *ptr, int size);



struct wsn_data_s * roll_wsn_data(void *ptr, int size)
{
    struct wsn_data_s *wsn = NULL;
    struct raw_node_data_s *raw = NULL;
    int time_llh_size = sizeof(struct time_llh_s);
    int raw_data_size = sizeof(struct raw_node_data_s);

    wsn = (struct wsn_data_s *)malloc(sizeof(uint8_t) * size);
    if(NULL == wsn){
        fprintf(stderr,"can't allocate memory for wsn data\n");
        return NULL;
    }
    memcpy( (struct time_llh_s *)wsn, ptr, sizeof(struct time_llh_s));

    wsn->llist_wsn = create_list();
    for(int i=0; i < wsn->time_llh.number_of_nodes; i++){
        raw = (struct raw_node_data_s *)malloc(sizeof(struct raw_node_data_s));
        memcpy(raw, ptr + time_llh_size + i * raw_data_size, raw_data_size);
        push_front(wsn->llist_wsn, (void *)raw);
    }

    return wsn;
}

         
int add_rolled_wsn_data_to_ll(struct wsn_data_s *wsn)
{
    if(NULL == wsn || NULL == config_G.llist_dataset){
        return -1;
    }

    push_front(config_G.llist_dataset, wsn);
    return 0;
}


int get_ll_size(void)
{
    return size(config_G.llist_dataset);
}

// TODO: used time_t instead of struct tm while sending messages between
// server and client. sizeof(struct tm) differs between beagle bone black and
// 64 bit linode server
static void print_sensor_payload(struct wsn_data_s * wsn)
{
    struct raw_node_data_s *raw = NULL;
    struct tm tm;

    tm = wsn->time_llh.timestamp;

    printf("lng: %lld\n", wsn->time_llh.longitude);
    printf("lat: %lld\n", wsn->time_llh.latitude);
    printf("combiner ID: %d\n", wsn->time_llh.combinerID);
    printf("# of nodes: %d\n", wsn->time_llh.number_of_nodes);
    printf("time: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1,
                                        tm.tm_mday, tm.tm_hour, tm.tm_min,
                                        tm.tm_sec);

    for(int i=0; i < wsn->time_llh.number_of_nodes; i++){
        printf("sensor info # %d\n", i);
        raw = (raw_node_data_t *)get_node_data_at_index(wsn->llist_wsn, i);

        //printf("P:%+6ld\n", raw->pressure);
        printf("P:%zu\n", raw->pressure);
        printf("T2:%+3d\n", raw->temp);
        printf("X:%+6d | Y:%+6d | Z:%+6d\n",  raw->accel_x,
                                              raw->accel_y,
                                              raw->accel_z );
        printf("H: %.2f\n", raw->humidity);
        printf("L: %.2f\n", raw->luminosity);
        printf("IP: %s\n", raw->addr);
    }

    return;
}

//static void print_sensor_payload(struct sensor_info_s *sensorInfo)
//{
//
//        printf("P:%+6ld\n", sensorInfo->pressure);
//        printf("T2:%+3d\n", sensorInfo->temp);
//        printf("X:%+6d | Y:%+6d | Z:%+6d\n",  sensorInfo->accel_x,
//                                              sensorInfo->accel_y,
//                                              sensorInfo->accel_z );
//        printf("H: %.2f\n", sensorInfo->humidity);
//        printf("L: %.2f\n", sensorInfo->luminosity);
//}

int prep_raw_data_file(struct wsn_data_s *wsn)
{
    //float zeros_13 = 10000000000000.0;
    long long lng = wsn->time_llh.longitude;
    long long ltd = wsn->time_llh.latitude;
    double lng_f = lng / 10000000000.;
    double ltd_f = ltd / 10000000000.;
    struct tm tm = wsn->time_llh.timestamp;
    struct raw_node_data_s *raw = NULL;
    
    config_G.fp_raw = fopen( rawDataFile, "w");
    fprintf(config_G.fp_raw,"INSERT INTO raw (the_geom, latitude, longitude, ax, ay, az, temp, pres, humidity, luminosity, combiner, time) VALUES\n");
    printf("INSERT INTO raw (the_geom, latitude, longitude, ax, ay, az, temp, pres, humidity, luminosity, combiner, time) VALUES\n");
    for(int i=0; i < size(wsn->llist_wsn) ; i++){

        raw = get_node_data_at_index(wsn->llist_wsn, i);

        fprintf(config_G.fp_raw,"(ST_SetSRID(ST_GeomFromGeoJSON('{\"type\":\"Point\",\"coordinates\":[%.8f,%8f]}'),4326),", lng_f, ltd_f );
        printf("(ST_SetSRID(ST_GeomFromGeoJSON('{\"type\":\"Point\",\"coordinates\":[%.8f,%8f]}'),4326),", lng_f, ltd_f );
        //fprintf(config_G.fp_raw,"%.8f,%.8f, %+6d, %+6d, %+6d, %+3d, %+6ld, %.2f, %d, %d:%d:%d)",          
        fprintf(config_G.fp_raw,"%lld,%lld, %+6d, %+6d, %+6d, %+3d, %zu, %.2f, %.2f, %d, '%d:%d:%d')",     \
                                ltd, lng, raw->accel_x, raw->accel_y, raw->accel_z, raw->temp,             \
                                raw->pressure, raw->humidity, raw->luminosity, wsn->time_llh.combinerID,   \
                                tm.tm_hour, tm.tm_min, tm.tm_sec);

        printf("%lld,%lld, %+6d, %+6d, %+6d, %+3d, %zu, %.2f, %.2f, %d, %d:%d:%d)",     \
                                ltd, lng, raw->accel_x, raw->accel_y, raw->accel_z, raw->temp,             \
                                raw->pressure, raw->humidity, raw->luminosity, wsn->time_llh.combinerID,   \
                                tm.tm_hour, tm.tm_min, tm.tm_sec);

        if(i == (size(wsn->llist_wsn) - 1)){
            fprintf(config_G.fp_raw,"\n;\n");
            printf("\n;\n");
        }else{
            fprintf(config_G.fp_raw,",\n");
            printf(",\n");
        }

        fflush(config_G.fp_raw);
        usleep(100);
    }
    fclose(config_G.fp_raw);


    return 0;
}

int init_stuff(void)
{
    // initialize everything
    //config_G.fp_raw = fopen( rawDataFile, "w");
    config_G.fp_geo = fopen( geoLocationFile, "w");
    config_G.llist_dataset = create_list();
    return 0;
}

int dismiss_stuff(void)
{
    // deinitialize everything
    //fclose(config_G.fp_raw);
    fclose(config_G.fp_geo);
    empty_list(config_G.llist_dataset, free_ll_wsn_history);

    return 0;
}

static void free_ll_raw_data(void *data)
{
    free(data);
    return;
}

static void free_ll_wsn_history(void *data)
{
    struct wsn_data_s *wsn_dp = data;

    empty_list(wsn_dp->llist_wsn, free_ll_raw_data);
    free(wsn_dp);
    return;
}

static void error(const char *msg)
{
    perror(msg);
    exit(1);
    return;
}

int main(int argc, char *argv[])
{
     struct wsn_data_s *wsn = NULL;
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[maxPayloadSize];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     //if (argc < 2) {
     //    fprintf(stderr,"ERROR, no port provided\n");
     //    exit(1);
     //}

     struct test_s{
        int16_t x;
        int16_t y;
        int64_t z;
        long long j;
        long long k;
        float l;
        float m;
     };

     struct test_s *test = NULL;

     printf("sizeof long long %d\n", sizeof(long long));
     printf("sizeof float %d\n", sizeof(float));
     printf("sizeof double %d\n", sizeof(double));
     printf("sizeof long %d\n", sizeof(long));
     printf("sizeof int %d\n", sizeof(int));
     printf("sizeof raw_node : %d\n", sizeof(struct raw_node_data_s));
     printf("sizeof time_llh : %d\n", sizeof(struct time_llh_s));
     printf("sizeof struct tm: %d\n", sizeof(struct tm));

     init_stuff();
     sockfd = socket(AF_INET, SOCK_STREAM, 0);

     if (sockfd < 0){
        error("ERROR opening socket");
     }
     //portno = atoi(argv[1]);
     portno = PORT_NO;

     bzero((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if( bind( sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
         error("ERROR on binding");
         return -1;
     }

     if( -1 ==  listen(sockfd, 5)){
         error("can't listen\n");
         return -1;
     }

     for(;;){

         clilen = sizeof(cli_addr);
         printf("waiting for a connection\n");
         if( 0 > (newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen)) ){
             error("can't accept");
         }

         do{

             bzero(buffer,maxPayloadSize);
             n = read(newsockfd,buffer,maxPayloadSize);

             if( n > 0){ // we've received data
                 printf("Message size: %d\n",n);
                 //n = write(newsockfd,"I got your message",18);
                 //if (n < 0) { error("ERROR writing to socket"); }
                 
                 test = (struct test_s *)buffer;

                 //printf("x: %d\n", test->x);
                 //printf("y: %d\n", test->y);
                 //printf("z: %zu\n", test->z);
                 //printf("j: %llu\n", test->j);
                 //printf("k: %llu\n", test->k);
                 //printf("l: %f\n", test->l);
                 //printf("m: %f\n", test->m);
                 printf("****************\n");

                 wsn = roll_wsn_data( (void *)buffer, n);
                 print_sensor_payload(wsn);
                 add_rolled_wsn_data_to_ll(wsn);
                 prep_raw_data_file(wsn);
                 system("./insert.sh");
                 free(wsn);
                 wsn = NULL;

             }
             else if( n < 0){
                 // error("seems like an error case");
             }else if( 0 == n ){
                 fprintf(stderr, "seems like opposing side has closed the connection\n");
                 break;
             }

             //sleep(1);
         } while(1);

         close(newsockfd);

     }

     close(sockfd);
     dismiss_stuff();
     return 0; 
}
