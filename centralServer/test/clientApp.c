/*
 ** echoc.c -- the echo client for echos.c; demonstrates unix sockets
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "pack.h"
#include "cfg.h"
#include "msgformat.h"
#include "cJSON.h"

#define SOCK_PATH "../sockIO/socketApp"

int main(void)
{
    int s, t, len, status;
    struct sockaddr_un remote;
    uint8_t rxbuf[200];

    struct packet_s* pack2Parse = NULL;
    struct packet_s* pack2Send = NULL;
    struct cfg_s* cfg = NULL;


    pack2Parse = (struct packet_s *)malloc(sizeof(struct packet_s));
    if(NULL == pack2Parse){
        return -1;
    }

start:
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    while(1){
        if (-1 == connect(s, (struct sockaddr *)&remote, len) ) {
            perror("connect");
            sleep(1);
            //exit(1);
        }else{
            break;
        }
    }

    printf("Connected.\n");
    status = 1;

    do{

        pack2Send = genCfg_Req();
        fprintf(stderr, "sending %d bytes with dac %d to server\n", pack2Send->len, pack2Send->dac);
        if(send(s, pack2Send, pack2Send->len + CHUNKCORELENGTH, 0) == -1){
            perror("send");
            //exit(1);
        }

        // Note that actually t should be larger than 4 hence chunk_core_length)
        if((t = recv(s, rxbuf, 200, 0)) > 0){

            fprintf(stderr, "incoming bytes len from server: %d\n", t);
            pack2Parse->data = (uint8_t *)malloc(sizeof(uint8_t) * (t - CHUNKCORELENGTH));
            pack2Parse->dac  = (uint16_t)rxbuf[0];
            pack2Parse->len  = (uint16_t)rxbuf[2];
            fprintf(stderr, "dac: %d\n", pack2Parse->dac);
            fprintf(stderr, "len: %d\n", pack2Parse->len);
            memcpy(pack2Parse->data, &rxbuf[CHUNKCORELENGTH], pack2Parse->len);

            switch(pack2Parse->dac){

                case DAC_CFG_CHANGE_NOTIFIER:
                    fprintf(stderr, "DAC_CFG_CHANGE_NOTIFIER came\n");
                    break;

                case DAC_CFG_REQ:
                    fprintf(stderr, "DAC_CFG_REQ came\n");
                    break;

                case DAC_CFG_RES:
                    fprintf(stderr, "DAC_CFG_RES came\n");
                    cfg = (struct cfg_s *)(pack2Parse->data);
                    printCfg(cfg);
                    cfg = NULL;

                    break;

                case DAC_CFG_MDFY:
                    fprintf(stderr, "DAC CFG MDFY came\n");
                    break;

                case DAC_SYS_DBG:
                    fprintf(stderr, "DAC SYS DBG came\n");
                    break;

                default:
                    fprintf(stderr, "Unknown DAC\n");
                    break;
            } //switch(pack2Parse->dac)

        } else if(t < 0){   // if((t = recv(s, rxbuf, 200, 0)) > 0)
            perror("recv"); 
        } else {
            printf("server closed connection\n");
            //exit(1);
            status = 0;
        }  // if((t = recv(s, rxbuf, 200, 0)) > 0)

    }while(status);

    sleep(6);



    close(s);
    goto start;

    return 0;
}











