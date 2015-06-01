#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cloud.h"

// Definitions
#define PORT_NO  51717
#define SERVER_IP "178.79.179.186"


// Global variables
int sockfd_G = 0;

// Private function prototypes
static void error(const char *msg);



static void error(const char *msg)
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
    memcpy( (char *) &serv_addr.sin_addr.s_addr,
            (char *) &server->h_addr,
            server->h_length);

    serv_addr.sin_port = htons(portno);

    if( connect(sockfd_G, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR connecting");
        return -1;
    }
    return 0;
}

int kill_connection_2_cloud(void)
{
    printf("killing connection to cloud\n");
    close(sockfd_G);
    return 0;
}


int send_data_2_cloud(void *buf, int size)
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














