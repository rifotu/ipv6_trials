#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

static const int  port = 2137;
static const char *ip = "127.0.0.1";
static int rtk_socket_G;

int connect_to_RTKlib(void)
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

void * read_frm_RTKlib(void){



    if( 0 > n = read(rtk_socket_G, 





















   
    
