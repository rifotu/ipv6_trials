/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

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
    unsigned char addr[16]; /* IPv6 address */
}raw_node_data_t;

typedef struct wsn_data_s{
    struct time_llh_s time_llh;
    list* llist_wsn;         // holds raw data for each sensor in the network 
}wsn_data_t;

typedef struct time_llh_s{
    int combinerID;
    int number_of_nodes;
    struct tm timestamp;
    long long latitude;
    long long longitude;
    long long height;
}time_llh_t;

struct wsn_data_t * roll_wsn_data(void *ptr, int size)
{
    struct wsn_data_s *wsn = NULL;
    struct raw_node_data_s *raw = NULL;
    int time_llh_size = sizeof(struct time_llh_s);
    int raw_data_size = sizeof(struct raw_node_data_s);

    if(NULL == wsn = (struct wsn_data_t *)malloc(sizeof(uint8_t) * size)){
        fprintf(stderr,"can't allocate memory for wsn data\n");
        return NULL;
    }
    memcpy( (struct time_llh_s *)wsn, ptr, sizeof(struct time_llh_s));

    create_list(wsn->llist_wsn);
    for(int i=0; i < wsn->time_llh.number_of_nodes; i++){
        raw = (struct raw_node_data_s *)malloc(sizeof(struct raw_node_data_s *));
        memcpy(raw, ptr + time_llh_size + i * raw_data_size, raw_data_size);
        push_front(raw, wsn->llist_wsn);
    }

    return wsn;
}

         
int add_rolled_wsn_data_to_ll(struct wsn_data_t *wsn)
{
    if(NULL == wsn || NULL == config_G.llist_dataset_G){
        return -1;
    }

    push_front(wsn, config_G.llist_dataset);
    return 0;
}


int get_ll_size(void)
{
    return size(config_G.llist_dataset);
}

static void print_sensor_payload(struct sensor_info_s *sensorInfo)
{

        printf("P:%+6ld\n", sensorInfo->pressure);
        printf("T2:%+3d\n", sensorInfo->temp);
        printf("X:%+6d | Y:%+6d | Z:%+6d\n",  sensorInfo->accel_x,
                                              sensorInfo->accel_y,
                                              sensorInfo->accel_z );
        printf("H: %.2f\n", sensorInfo->humidity);
        printf("L: %.2f\n", sensorInfo->luminosity);
}

int prep_raw_data_file(struct wsn_data_t *wsn)
{
    float lng = wsn->time_llh.longitude;
    float ltd = wsn->time_llh.latitude;
    struct tm tm = wsn->time_llh.timestamp;
    struct raw_node_data_s *raw = NULL;
    
    
    fprintf(config_G.fp_raw,"INSERT INTO raw (the_geom, latitude, longitude, ax, ay,
                                              az, temp, pres, humidity, luminosity,
                                              combiner, time) VALUES\n");
    for(int i=0; i < size(wsn->llist_wsn) ; i++){

        raw = get_node_data_at_index(wsn->llist_wsn, i);

        fprintf(config_G.fp_raw,"ST_SetSRID(ST_GeomFromGeoJSON('{\"type\":\"Point\",\"coordinates\":[%.8f,%8f]}'),4326),", ltd, lng );
        fprintf(config_G.fp_raw,"%.8f,%.8f, %+6d, %+6d, %+6d, %+3d, %+6ld, %.2f, %d, %d:%d:%d)",
                                ltd, lng, raw->accel_x, raw->accel_y raw_accel_z, raw->temp,
                                raw->pressure, raw->humidity, raw->luminosity, wsn->time_llh.combinerID,
                                tm.tm_hour, tm.tm_min, tm.tm_sec);

        if(i == size(wsn->llist_wsn) - 1){
            fprintf(config_G.fp_raw,",\n");
        }else{
            fprintf(config_G.fp_raw,"\n");
        }
        fprintf(config_G.fp_raw,";");
    }
}

int init_stuff(void)
{
    // initialize everything
    config_G.fp_raw = fopen( rawDataFile, "w");
    config_G.fp_geo = fopen( geoLocationFile, "w");
    create_list(config_G.llist_dataset);
    return 0;
}

int dismiss_stuff(void)
{
    // deinitialize everything
    fclose(config_G.fp_raw);
    fclose(config_G.fp_geo);
    empty_list(config_G.llist_dataset, free_ll_wsn_history)

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

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[maxPayloadSize];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);

     if (sockfd < 0){
        error("ERROR opening socket");
     }
     portno = atoi(argv[1]);

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
                 printf("Here is the message: %s\n",buffer);
                 n = write(newsockfd,"I got your message",18);
                 if (n < 0) { error("ERROR writing to socket"); }
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
     return 0; 
}
