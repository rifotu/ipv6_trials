//UDPClient.c
 
/*
 * gcc -o client UDPClient.c
 * ./client <server-ip>
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
//#define BUFLEN 30
//#define PORT 9930
#define PORT 3000


struct sensor_info_s{
        int16_t accel_x;
        int16_t accel_y;
        int16_t accel_z;
        uint16_t temp;
        uint32_t pressure;
        float humidity;
        float luminosity;
}; 

static void print_sensor_payload(struct sensor_info_s *sensorInfo);

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

void err(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char** argv)
{
    struct sockaddr_in6 serv_addr;
    int sockfd, i, slen=sizeof(serv_addr);
    char buf[BUFLEN];

    int len;

    unsigned int fromSize; /* In - out of address size for recvfrom() */
    struct sockaddr_in6 fromAddr;  /* Source address of echo */
    int respStringLen;  /* Length of received response */
    char echoBuffer[BUFLEN + 1];
 

    len = sizeof(struct sensor_info_s);

    if(argc != 2)
    {
      printf("Usage : %s <Server-IP>\n",argv[0]);
      exit(0);
    }
 
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) <= 0)
        err("socket");
 
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(PORT);
    if (inet_pton(AF_INET6, argv[1], &serv_addr.sin6_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
 
    while(1)
    {
        printf("\nEnter data to send(Type exit and press enter to exit) : ");
        scanf("%[^\n]",buf);
        getchar();
        if(strcmp(buf,"exit") == 0){
          exit(0);
        }
 
        if (sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&serv_addr, slen)==-1){
            err("sendto()");
        }

20 Nisan Antsis Elektronik

        fromSize = sizeof(fromAddr);
        if(respStringLen = recvfrom(sockfd, echoBuffer, len, 0, 
                                    (struct sockaddr *)&fromAddr, &fromSize) == -1){
                err("recvfrom()");
        }
        
        print_sensor_payload( (struct sensor_info_s *)echoBuffer);

    }
 
    close(sockfd);
    return 0;
}
