#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "sensorNode.h"
#include "gpxlogger.h"
#include "list.h"


#define COMBINER_ID  87


// Constants
const char ipv6_addresses_G[2][25] = {
    {"aaaa::212:4b00:43c:4d03"},
    {"aaaa::212:4b00:43c:4be5"}
};

static const int PORT_NUMBER = 3000;
static const float zeros_13 = 10000000000.0;
static const float zeros_4 = 10000.0;


// Structure definitions
typedef struct raw_node_data_lcd_s{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint16_t temp;
    uint32_t pressure;
    uint16_t humidity;
    uint16_t luminosity;
    unsigned char addr[25];
}raw_node_data_lcd_t;

typedef struct time_llh_lcd_s{
    int combinerID;
    int number_of_nodes;
    long long latitude;
    long long longitude;
}time_llh_lcd_t;

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
    int combinerID;    // TODO: this should actually be in struct wsn_info_s
    int number_of_nodes;
    struct tm timestamp;
    uint8_t dummy[12];  // TODO: quick fix, remove this by changin struct tm to time_t
    long long latitude;
    long long longitude;
    long long height;
}time_llh_t;

typedef struct wsn_data_s{
    struct time_llh_s time_llh;
    list* llist_wsn;         // holds raw data for each sensor in the network 
}wsn_data_t;

// holds information regarding 
// the number of nodes and their
// properties
typedef struct wsn_info_s{
    int number_of_nodes;
    list *ll_node_prop; 
    //TODO: Determine further parameters
}wsn_info_t;


/* holds information regarding
   how to communicate with sensor
   nodes */
typedef struct node_prop_s{
    char ipv6_address[25];
    int port;
    int type;
    //TODO: see what you can find
}node_prop_t;

typedef struct wsn_config_s{
    wsn_info_t *wsn_info;
    int readout_period;
    list *ll_wsn_data_history;
    //TODO:  Determine further parameters
}wsn_config_t;

// Global Variables
struct wsn_config_s *wsn_config_G;

// Private Function Prototypes
static void timestamp(struct wsn_data_s *wsn);
static int geolocate(struct wsn_data_s *wsn);
static void * get_wsn_info(void);
static void free_ll_node_prop(void *data);
static void free_ll_raw_data(void *data);
static void free_ll_wsn_history(void *data);
static void * get_mem_for_raw_data(void);
static size_t size_of_raw_data(void);
static void append_node_id(void *str, struct in6_addr *addr);
static void * get_data_from_sensor(char *ip_addr, int port);
static void print_sensor_payload(struct raw_node_data_s *sensorInfo);
//static uint16_t cnvt_2_int(float val);


static void timestamp(struct wsn_data_s *wsn)
{
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);

    fprintf(stdout,"now: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1,
                                               tm.tm_mday, tm.tm_hour, tm.tm_min,
                                               tm.tm_sec);
    wsn->time_llh.timestamp = tm;
    return;
}

static int geolocate(struct wsn_data_s *wsn)
{
    //if(NULL == read_frm_RTKlib()){
    //    fprinf(stderr, " can't read from RTKli\n");
    //    return -1;
    //}

    wsn->time_llh.latitude = get_latitude();
    wsn->time_llh.longitude = get_longitude();
    wsn->time_llh.height = get_height();
    return 0;
}

// Currently this function will not be used as
// each dataset will be sent immediately and
// won't be stored
void record_a_set_of_wsn_data(void *ptr)
{
    struct wsn_data_s *wsn_data = ptr;

    push_front(wsn_config_G->ll_wsn_data_history, wsn_data);
    
    // check whether we have passed the recording limit
    if(20 <= size(wsn_config_G->ll_wsn_data_history) ){
        remove_back(wsn_config_G->ll_wsn_data_history, free_ll_wsn_history);
    }

    return;
}


void * prep_a_set_of_wsn_data(void)
{
    struct wsn_data_s *wsn = NULL;
    struct node_prop_s *prop = NULL;
    void *raw_data;

    // TODO
    // get number of wsn nodes & correspending addresses
    // Go into a for loop {
    // Timestamp & Geolocate data set

    wsn = (struct wsn_data_s *)malloc(sizeof(struct wsn_data_s));
    wsn->llist_wsn = create_list();

    for(int i=0; i < wsn_config_G->wsn_info->number_of_nodes; i++){
        prop = (struct node_prop_s *)get_node_data_at_index(wsn_config_G->wsn_info->ll_node_prop, i);
        raw_data = get_data_from_sensor(prop->ipv6_address, prop->port);
        push_front(wsn->llist_wsn, raw_data);
        usleep(100);
    }

    //TODO
    // interpret get_data_from_sensor output and determine the number of nodes in the payload
    // for now it is hardcoded to 2 but it can be lower since recvfrom return resource not available
    // from time to time
    wsn->time_llh.number_of_nodes = 2;
    wsn->time_llh.combinerID = COMBINER_ID;

    memset(wsn->time_llh.dummy, 0, sizeof(12)); // TODO: remove this, related to quick fix

    timestamp(wsn);
    geolocate(wsn);

    return (void *)wsn;
}

int calc_size_for_unrolled_data(void *ptr)
{
    int msize = 0;
    if(NULL == ptr){ return -1; }

    struct wsn_data_s *wsn = (struct wsn_data_s *)ptr;

    msize = sizeof(struct time_llh_s) + 
            sizeof(raw_node_data_t) * wsn->time_llh.number_of_nodes;

    return msize;
}

void print_cloud_data(uint8_t *buf)
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




int calc_size_for_unrolled_lcd_data(void *ptr)
{
    int msize = 0;
    if(NULL == ptr){return -1; }

    struct wsn_data_s *wsn = ptr;

    msize = sizeof(struct time_llh_lcd_s) +
            sizeof(raw_node_data_lcd_t) * wsn->time_llh.number_of_nodes;

    return msize;
}
    

void * unroll_wsn_data(void *ptr)
{
    uint8_t *unrolled_p = NULL;
    struct wsn_data_s *wsn = ptr;
    int msize = 0;
    int offset = 0;

    msize = calc_size_for_unrolled_data(wsn);

    unrolled_p = (uint8_t *)malloc(sizeof(uint8_t) * msize);

    memcpy(unrolled_p, wsn, sizeof(struct time_llh_s));

    for(int i=0; i < wsn->time_llh.number_of_nodes; i++){

        offset = sizeof(struct time_llh_s) + i * sizeof(struct raw_node_data_s);

        memcpy( unrolled_p + offset,
                (void *)get_node_data_at_index(wsn->llist_wsn, i),
                sizeof(struct raw_node_data_s));

    }

    return (void *)unrolled_p;
}

//static uint16_t cnvt_2_int(float val)
//{
//    val *= zeros_4;
//    return (uint16_t)val;
//}


void * unroll_wsn_data_lcd(void *ptr)
{
    uint8_t *unrolled_p = NULL;
    struct wsn_data_s *wsn = ptr;
    struct time_llh_lcd_s llh_lcd; 
    struct raw_node_data_s *raw_data = NULL;
    struct raw_node_data_lcd_s raw_data_lcd;
    int msize = 0;
    int offset = 0;

    msize = calc_size_for_unrolled_data(wsn);
    unrolled_p = (uint8_t *)malloc(sizeof(uint8_t) * msize);

    llh_lcd.combinerID = wsn->time_llh.combinerID;
    llh_lcd.number_of_nodes = wsn->time_llh.number_of_nodes; 
    llh_lcd.latitude = wsn->time_llh.latitude;
    llh_lcd.longitude = wsn->time_llh.longitude;

    memcpy(unrolled_p, &llh_lcd, sizeof(struct time_llh_lcd_s));

    for(int i=0; i < wsn->time_llh.number_of_nodes; i++){

        offset = sizeof(struct time_llh_lcd_s) + i * sizeof(struct raw_node_data_lcd_s);

        raw_data = (struct raw_node_data_s *)get_node_data_at_index(wsn->llist_wsn, i);

        raw_data_lcd.accel_x  = raw_data->accel_x;
        raw_data_lcd.accel_y  = raw_data->accel_y;
        raw_data_lcd.accel_z  = raw_data->accel_z;
        raw_data_lcd.temp     = raw_data->temp;
        raw_data_lcd.pressure = raw_data->pressure;
        raw_data_lcd.humidity = raw_data->humidity;
        memcpy(raw_data_lcd.addr, 
               raw_data->addr, 
               sizeof(unsigned char)*16  );
    
        memcpy( unrolled_p + offset,
                (const void *) &raw_data_lcd,
                sizeof(struct raw_node_data_lcd_s) );
    }

    return (void *)unrolled_p;
}

static void * get_wsn_info(void)
{
    struct wsn_info_s *wsn_info_p = NULL;
    struct node_prop_s *node_prop_p = NULL;
    // Get the number of nodes in the system
    // from the border router
    // Get the corresponding addresses


    wsn_info_p = (struct wsn_info_s *)malloc(sizeof(struct wsn_info_s));
    wsn_info_p->number_of_nodes = 2;
    wsn_info_p->ll_node_prop = create_list();

    // Currently node address are hardcoded
    for(int i=0; i < wsn_info_p->number_of_nodes; i++){
        node_prop_p = malloc(sizeof(struct node_prop_s)); 
        memcpy(node_prop_p->ipv6_address, ipv6_addresses_G[i], sizeof(char)*25);
        node_prop_p->type = 2; // 2 is a random choice
        node_prop_p->port = PORT_NUMBER;
        push_front( wsn_info_p->ll_node_prop, node_prop_p);
    }

    return (void *)wsn_info_p;
    
}


int initiate_wsn(void)
{
    printf("init wsn called\n");
    //TODO: have the tunslip interface up
    wsn_config_G = malloc(sizeof(struct wsn_config_s));
    wsn_config_G->readout_period = 5;  // in terms of minutes
    wsn_config_G->wsn_info = (wsn_info_t *)get_wsn_info();
    wsn_config_G->ll_wsn_data_history = create_list();
    printf("init wsn returns\n");
    return 0;

}

int dismiss_wsn(void)
{
    //TODO: have the tunslip interface down
    empty_list(wsn_config_G->wsn_info->ll_node_prop, free_ll_node_prop);
    empty_list(wsn_config_G->ll_wsn_data_history, free_ll_wsn_history);
    free(wsn_config_G->wsn_info);
    free(wsn_config_G);
    return 0;
}

static void free_ll_node_prop(void *data)
{
    free(data);
    return;
}

static void free_ll_raw_data(void *data)
{
    free(data);
    return;
}

static void free_ll_wsn_history(void *data)
{
    struct wsn_data_s *wsn_dp = data;   // assign wsn data pointer

    empty_list(wsn_dp->llist_wsn, free_ll_raw_data);
    free(wsn_dp);
}


static void * get_mem_for_raw_data(void)
{

    struct raw_node_data_s* raw_buf = NULL;

    raw_buf = (struct raw_node_data_s *)malloc(sizeof(struct raw_node_data_s));
    memset(raw_buf, 0, sizeof(struct raw_node_data_s));

    return raw_buf;
}

static size_t size_of_raw_data(void)
{
    return sizeof(struct raw_node_data_s);
}

static void append_node_id(void *ptr, struct in6_addr *addr)
{
    char *str = ((struct raw_node_data_s *)ptr)->addr;
//  sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
    sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
            (int)addr->s6_addr[0], (int)addr->s6_addr[1],
//          (int)addr->s6_addr[2], (int)addr->s6_addr[3],
//          (int)addr->s6_addr[4], (int)addr->s6_addr[5],
//          (int)addr->s6_addr[6], (int)addr->s6_addr[7],
            (int)addr->s6_addr[8], (int)addr->s6_addr[9],
            (int)addr->s6_addr[10], (int)addr->s6_addr[11],
            (int)addr->s6_addr[12], (int)addr->s6_addr[13],
            (int)addr->s6_addr[14], (int)addr->s6_addr[15]);

}


//TODO: Move this function to a new source file for socket operations
int setNonBlocking(int fd)
{
      int flags;


      /* if O_NONBLOCK is defined, use the fcntl function */
        if(-1 == (flags = fcntl(fd, F_GETFL, 0))){
                flags = 0;
                  }

          return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// IPv6 communication with the sensor nodes
static void * get_data_from_sensor(char *ip_addr, int port)
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
    serv_addr.sin6_port = htons(port);

    // convert IPv4 and IPv6 addresses from text to binary form 
    if( 0 == inet_pton(AF_INET6, ip_addr, &serv_addr.sin6_addr)){
        fprintf(stderr, "inet_aton() failed\n");
    }

    if( -1 == sendto(sockfd, dummy_msg, strlen(dummy_msg), 0, (struct sockaddr *) &serv_addr, slen) ){
        printf("sendto() function");
    }
    
    raw_buf = get_mem_for_raw_data(); //TODO: check for null pointer return

    // set a timeout for recvfrom function
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 400000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }

    if( 0 >= ( resp_buf_len = recvfrom(sockfd, raw_buf, size_of_raw_data(), 0,
                                       (struct sockaddr *) &resp_addr, &resp_addr_size   ))){
        printf("recvfrom returned: %d\n", resp_buf_len);
        perror("Error");
    }


    append_node_id( raw_buf, &(serv_addr.sin6_addr));

    //print_sensor_payload((struct raw_node_data_s *)raw_buf);
    close(sockfd);

    return raw_buf;
}


static void print_sensor_payload(struct raw_node_data_s *sensorInfo)
{

    printf("P:%d\n", sensorInfo->pressure);
    printf("T2:%+3d\n", sensorInfo->temp);
    printf("X:%+6d | Y:%+6d | Z:%+6d\n",  sensorInfo->accel_x,
            sensorInfo->accel_y,
            sensorInfo->accel_z );
    printf("H: %.2f\n", sensorInfo->humidity);
    printf("L: %.2f\n", sensorInfo->luminosity);
    printf("IP: %s\n", sensorInfo->addr);
}













