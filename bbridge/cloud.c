#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Definitions
#define PORT_NO  51717
#define SERVER_IP "178.79.179.186"


// Global variables
int sockfd_G = 0;

// Private function prototypes
void error(const char *msg);

// Public function prototypes
int initiate_connection_2_cloud(void);
int kill_connection_2_cloud(void);
int send_data_2_cloud(void *buf, int size);


void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int initiate_connection_2_cloud(void)
{
    int portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    
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
    memcpy( (char *) &serv_addr.sin_addr.s.addr,
            (char *) &server->h_addr,
            server->h_length);

    serv_addr.sin_port = htons(portno);

    if( connect(sockfd_G, (struct sockadr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR connecting");
        return -1;
    }
}

int kill_connection_2_cloud(void)
{
    printf("killing connection to cloud\n";
    close(sockfd_G);
}


int send_data_2_cloud(void *buf, int size)
{
    int n = 0;
    printf("sending data to cloud\n");
    n = write(sockfd, buf, size);
    if( n < 0){
        error("error writing to socket");
        return -1;
    }

    return 0;
}














