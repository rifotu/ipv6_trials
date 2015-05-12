#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
}

int kill_connection_2_cloud(void)
{
    printf("killing connection to cloud\n");
    close(sockfd_G);
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

int main(int argc, char *argv[])
{

    char buffer[256];
 
    printf("enter a message\n");
    bzero(buffer,256);
    fgets(buffer,255,stdin);

    initiate_connection_2_cloud();
    send_data_2_cloud(buffer, sizeof(buffer));
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
