#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

int connect_to_RTKlib(int port)
{
    int socket_fd;
    struct sockaddr_in name;
    
