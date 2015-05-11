#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#define COMBINER_ID  87

// Global Variables
struct config_s *wsn_config_G;

// Constants
const char ipv6_addresses_G[2][25] = {
    {"aaaa::212:4b00:433:edae"},
    {"aaaa::212:4b00:43c:4be5"}
};

const int PORT_NUMBER = 3000;

// Structure definitions
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
    float latitude;
    float longitude;
    float height;
}time_llh_t;

typedef struct wsn_info_s{
    int number_of_nodes;
    list *ll_node_prop; 
    //TODO: Determine further parameters
}wsn_info_t;

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


// Private Function Prototypes
static void timestamp(void *p);
static int geolocate(struct wsn_data_s *wsn);
static void * get_wsn_info(void);
static void free_ll_node_prop(void *data);
static void free_ll_raw_data(void *data);
static void free_ll_wsn_history(void *data);
static void * get_mem_for_raw_data(void);
static size_t size_of_raw_data(void);
static void *append_node_id(void *ptr, struct in6_addr sin6_addr);
static void * get_data_from_sensor(char *ip_addr, int port);


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
    if(NULL == read_frm_RTKlib()){
        fprinf(stderr, " can't read from RTKli\n");
        return -1;
    }

    wsn->time_llh.latitude = get_latitude();
    wsn->time_llh.longitude = get_longitude();
    wsn->time_llh.height = get_height();


}

void record_a_set_of_wsn_data(void *ptr)
{
    struct wsn_data_s *wsn_data = ptr;

    push_front(wsn_config_G->llist_wsn_data_history, wsn_data);
    
    // check whether we have passed the recording limit
    if(20 <= size(wsn_config_G->llist_wsn_data_history) ){
        remove_back(wsn_config_G->llist, free_ll_wsn_history);
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

    wsn = (struct wsn_data_s *)malloc(sizeof(struct wsn_data_s *));
    wsn->llist_wsn = create_list();

    for(int i=0; i < wsn_config_G->wsn_info->number_of_nodes; i++){
        prop = (struct node_prop_s *)get_node_data_at_index(wsn_config_G->wsn_info->ll_node_prop, i);
        raw_data = get_data_from_sensor(prop->ipv6_address, prop->port);
        push_front(wsn->llist_wsn, raw_data);
    }

    timestamp(wsn);
    geolocate(wsn);

    return (void *)wsn;
}

int calc_size_for_unrolled_data(void *ptr)
{
    if(NULL == ptr){ return -1; }

    struct wsn_data_s *wsn = ptr;

    msize = sizeof(struct time_llh_s) + 
            sizeof(raw_node_data_t) * wsn->time_llh.number_of_nodes;

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

    return (void *)wsn;
}





static void * get_wsn_info(void)
{
    struct wsn_info_s *wsn_info_p = NULL;
    struct node_prop_s *node_prop_p = NULL;
    // Get the number of nodes in the system
    // from the border router
    // Get the corresponding addresses


    wsn_info_p = malloc(sizeof(wsn_info_s));
    wsn_info_p->number_of_nodes = 2;
    wsn_info_p->ll_node_prop = create_list();

    // Currently node address are hardcoded
    for(int i=0; i < wsn_info_p->number_of_nodes; i++){
        node_prop_p = malloc(sizeof(struct node_prop_s)); 
        memcpy(node_prop_p->ipv6_address, ipv6_addresses[i], sizeof(char)*25);
        node_prop_p->type = 2; // 2 is a random choice
        node_prop_p->port = PORT_NUMBER;
        push_front( wsn_info_p->ll_node_prop, node_prop_p);
    }

    return (void *)wsn_info_p;
    
}


int initiate_wsn(void)
{
    //TODO: have the tunslip interface up
    wsn_config_G = malloc(sizeof(struct config_s));
    wsn_config_G->readout_period = 5;  // in terms of minutes
    wsn_config_G->wsn_info = (wns_info_t *)get_wsn_info();
    wsn_config_G->ll_wsn_data_history = create_list();
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
    memset(0, raw_buf);

    return raw_buf;
}

static size_t size_of_raw_data(void)
{
    return sizeof(struct raw_node_data_s);
}

static void *append_node_id(void *ptr, struct in6_addr sin6_addr)
{
    struct raw_node_data_s *node = ptr;

    memcpy(node->addr, sin6_addr.s6_addr, sizeof(unsigned char)*16);

    return ptr;
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


    append_node_id(raw_buf, &serv_addr.sin6_addr);

    //print_sensor_payload();
    close(sockfd);

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
















