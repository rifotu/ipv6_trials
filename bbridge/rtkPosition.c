#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

// Structure definitions
typedef struct llh_s{
    float longitude;
    float latitude;
    float height;
}llh_t;

// Global Variables
static const int  port = 2137;
static const char *ip = "127.0.0.1";
static int rtk_socket_G;
static llh_t *llh_G;

// Private functions
static int connect_to_rtklib(void)


int init_rtklib_client(void)
{
    llh_G = malloc(sizeof(struct llh_s));
    if(NULL == llh_G){
        return -1;
    }

    if(-1 == connect_to_rtklib() ){
        return -1;
    }

    return 0;
}

int dismiss_rtklib_client(void)
{
    free(llh_G);
    llh_G = NULL;

    close(rtk_socket_G);
    return 0;
}

static int connect_to_rtklib(void)
{
    struct sockaddr_in rtkLib_srv;

    if( 0 > ( rtk_socket_G = socket(AF_INET, SOCK_STREAM, 0)) ){
        fprintf(stderr, "could not create socket\n");
        return -1;
    }

    memset( &rtklib_srv, '0', sizeof(rtklib_srv));
    rtklib_srv.sin_family = AF_INET;
    rtklib_srv.sin_port   = htons(port);

    if( 0 >= inet_pton(AF_INET, ip, &rtklib_srv.sin_addr) <= 0)
    {
        fprintf(stderr, "\n inet_pton error occured\n");
        return -1;
    }

    if( 0 > connect( rtk_socket_G, (struct sockaddr *) &rtklib_srv, sizeof(struct sockaddr_in))){
        fprintf(stderr, "Error: connect failed\n");
        return -1;
    }
    
    return 0;
}

void * read_frm_RTKlib(void)
{
    if( 0 > n = read(rtk_socket_G, llh_G, sizeof(struct llh_s)) ){
        fprintf(stderr, "Error can't read\n");
        return NULL;
    }else{
        return (void *)llh_G;
    }
}

float get_latitude(void)
{
    if(NULL == llh_G){
        return -10000;
    }else{
        return (struct llh_s *)llh_G->latitude;
    }
}

float get_longitude(void)
{
    if(NULL == llh_G){
        return -10000;
    }else{
        return (struct llh_s *)llh_G->longitude;
    }
}

float get_height(void)
{
    if(NULL == llh_G){
        return -10000;
    }else{
        return (struct llh_s *)llh_G->height;
    }
}


    











































































   
    
