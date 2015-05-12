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

list *llist_dataset_G = NULL;

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
    if(NULL == wsn || NULL == llist_dataset_G){
        return -1;
    }

    push_front(wsn, llist_dataset_G);
    return 0;
}

int init_stuff(void)
{
    create_list(llist_dataset_G);
    return 0;
}

int dismiss_stuff(void)
{
    // empty stuff

}

static void * free







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
