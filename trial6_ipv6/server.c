//UDPServer.c
 
/*
 *  gcc -o server UDPServer.c
 *  ./server
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define BUFLEN 512
#define PORT 9930
 
void err(char *str)
{
    perror(str);
    exit(1);
}
 
int main(void)
{
    struct sockaddr_in6 my_addr, cli_addr;
    int sockfd, i;
    socklen_t slen=sizeof(cli_addr);
    char buf[BUFLEN];
    char ipAddrString[INET6_ADDRSTRLEN];
    char *ackMsg = "ack";
 
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("socket");
    else
      printf("Server : Socket() successful\n");
 
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin6_family = AF_INET6;
    my_addr.sin6_port = htons(PORT);
    my_addr.sin6_addr = in6addr_any;
     
    if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1)
      err("bind");
    else
      printf("Server : bind() successful\n");
 
    while(1)
    {
        if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&cli_addr, &slen)==-1){
            err("recvfrom()");
        }
        inet_ntop(AF_INET6, &(cli_addr.sin6_addr), ipAddrString, INET6_ADDRSTRLEN), 
        printf("Received packet from %s:%d\nData: %s\n\n",
                ipAddrString, ntohs(cli_addr.sin6_port), buf );

        memset(buf, 0, sizeof(buf));
        snprintf(buf, 4, "ackk");

        
        /* send ack datagram back to client */
        if( sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) == -1){
                err("sendto()");
        }
        printf("ack sent\n");




    }


 
    close(sockfd);
    return 0;
}
