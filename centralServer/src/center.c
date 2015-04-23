
/********************** INCLUDES ************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>


#include "list.h"
#include "pack.h"
#include "cfg.h"
#include "msgformat.h"


/**************** END OF INCLUDES **********************/

/******************* Prototypes ************************/
struct dataGenType_s** genConfigForThreads(const char** sockPaths, const int socketNo);
void freeLinkedListData(void *data);
void keepLinkedListData(void *data);
void* socketManager(void *arg);
int sendall(int conn_fd, uint8_t *buf, int size);
int setNonBlocking(int fd);
int socketSnmp_f(void *p);
int socketApp_f(void *p);
/****************** End of Prototypes ******************/

/********** Declarations & Global Variables ************/

typedef int (*dac_func)(void *packet);
struct dataGenType_s** dCfg_G = NULL;
//const int NOOFSOCKETS = 2;
#define NOOFSOCKETS 2
const int maxPayloadSize = 500;
const char* socketNames[NOOFSOCKETS] = {"../sockIO/socketApp", "../sockIO/socketSnmp"};
const dac_func functionNames[NOOFSOCKETS] = {socketApp_f, socketSnmp_f};

struct dataGenType_s{

  list*  llistRx;
  list*  llistTx;
  dac_func  function;  // function pointer
  const char* socketPath;
  uint8_t socketStatus;
  pthread_mutex_t mutex;

}dataGenType_t;
/***************** End of Declarations *****************/


// use this function when agent wants to send snmp or vice versa
int push_2_ll(char *ll_name, void *pack_p)
{
    int stat = 0;

    if( !strncmp(ll_name, "snmp", sizeof("snmp")) ){
        push_front( (dCfg_G[1]->llistTx), pack_p);
        stat = 0;
    }else if( !strncmp(ll_name, "app", sizeof("app")) ){
        push_front( (dCfg_G[0]->llistTx), pack_p);
        stat = 0;
    }else{
        stat = -1;
    }

    return stat;
}


// Function definitions
int socketSnmp_f(void *p)
{
    struct cfg_s* jsonCfg = NULL;
    struct dataGenType_s* threadCfg = NULL;
    struct packet_s* pack2Parse = NULL;
    struct packet_s* packRes = NULL;

    threadCfg = (struct dataGenType_s *)p;

    pack2Parse = (struct packet_s *) back(threadCfg->llistRx);
    if( (NULL == pack2Parse) || is_empty(threadCfg->llistRx) ){
        // no data in linked list
        //fprintf(stderr, "no data in linked list: %d\n", i);
    } else {

        switch(pack2Parse->dac){

            case  DAC_CFG_CHANGE_NOTIFIER:
                fprintf(stderr, "Error!! shouldn't have got this, DAC_CFG_CHANGE_NOTIFIER\n");
                break;

            case DAC_CFG_REQ:
                fprintf(stderr, "DAC_CFG_REQ came\n");
                jsonCfg = getCfg();
                printCfg(jsonCfg);
                packRes = genCfg_Res(jsonCfg);
                push_front(threadCfg->llistTx, packRes);
                //push_2_ll("app", packRes);

                //printCfg(  (struct cfg_s *)( packRes->data ) );
                packRes = NULL;
                pack2Parse = NULL;
                break;

            case DAC_CFG_RES:
                fprintf(stderr, "Error!! shouldn't have got this, DAC_CFG_RES\n");
                break;

            case DAC_CFG_MDFY:
                fprintf(stderr, "DAC CFG MDFY came\n");
                mdfCfg(pack2Parse);
                pack2Parse = NULL;
                break;

            case DAC_SYS_DBG:
                fprintf(stderr, "DAC SYS DBG came\n");
                fprintf(stderr, ">%s\n", pack2Parse->data);
                packRes = genSys_Dbg((char *)(pack2Parse->data));
                push_front(threadCfg->llistTx, packRes);

                packRes = NULL;
                pack2Parse = NULL;
                break;


            default:
                fprintf(stderr, "Unknown DAC\n");
                break;

        }  // switch(pack2Parse->dac)

        printf("agent: ll size: %d\n", size(threadCfg->llistRx));
        remove_back(threadCfg->llistRx, freeLinkedListData);
        printf("agent: ll size: %d\n", size(threadCfg->llistRx));
    } // if( (NULL == llbuf) || is_empty(threadCfg[i]->llistRx) )

    usleep(5000);
    return 0;
}

int socketApp_f(void *p)
{
    struct cfg_s* jsonCfg = NULL;
    struct dataGenType_s* threadCfg = NULL;
    struct packet_s* pack2Parse = NULL;
    struct packet_s* packRes = NULL;

    threadCfg = (struct dataGenType_s *)p;

    pack2Parse = (struct packet_s *) back(threadCfg->llistRx);
    if( (NULL == pack2Parse) || is_empty(threadCfg->llistRx) ){
        // no data in linked list
        //fprintf(stderr, "no data in linked list: %d\n", i);
    } else {

        switch(pack2Parse->dac){

            case  DAC_CFG_CHANGE_NOTIFIER:
                fprintf(stderr, "Error!! shouldn't have got this, DAC_CFG_CHANGE_NOTIFIER\n");
                break;

            case DAC_CFG_REQ:
                fprintf(stderr, "DAC_CFG_REQ came\n");
                jsonCfg = getCfg();
                printCfg(jsonCfg);
                packRes = genCfg_Res(jsonCfg);
                push_front(threadCfg->llistTx, packRes);

                //printCfg(  (struct cfg_s *)( packRes->data ) );
                packRes = NULL;
                pack2Parse = NULL;
                break;

            case DAC_CFG_RES:
                fprintf(stderr, "Error!! shouldn't have got this, DAC_CFG_RES\n");
                break;

            case DAC_CFG_MDFY:
                fprintf(stderr, "DAC CFG MDFY came\n");
                mdfCfg(pack2Parse);
                pack2Parse = NULL;
                break;

            case DAC_SYS_DBG:
                fprintf(stderr, "DAC SYS DBG came\n");
                fprintf(stderr, ">%s\n", pack2Parse->data);
                packRes = genSys_Dbg((char *)(pack2Parse->data));
                push_front(threadCfg->llistTx, packRes);

                packRes = NULL;
                pack2Parse = NULL;
                break;


            default:
                fprintf(stderr, "Unknown DAC\n");
                break;

        }  // switch(pack2Parse->dac)

        printf("App: ll size: %d\n", size(threadCfg->llistRx));
        remove_back(threadCfg->llistRx, freeLinkedListData);
        printf("App: ll size: %d\n", size(threadCfg->llistRx));
    } // if( (NULL == llbuf) || is_empty(threadCfg[i]->llistRx) )

    usleep(5000);
    return 0;
}


int main(void)
{

    pthread_t idThreadSockets[NOOFSOCKETS];

    prepCfgStruct();
    dCfg_G = genConfigForThreads(socketNames, NOOFSOCKETS);

    for(int i=0; i<NOOFSOCKETS; i++){
        pthread_create(&idThreadSockets[i], NULL, socketManager, (void *)dCfg_G[i]);
    }

    fprintf(stderr, "going into while loop in main\n");

    while(1){

        // parse through linked lists and take necessary measures
        for(int i=0; i<NOOFSOCKETS; i++){

            dCfg_G[i]->function( (void *)dCfg_G[i] );
            sleep(2);
        } // for(int i=0; i<NOOFSOCKETS; i++){
    } // while(1){

} // End of "int main(void)"


//TODO: Move this function to a new source file for socket operations
int setNonBlocking(int fd)
{
  int flags;


/* if O_NONBLOCK is defined, use the fcntl function */
  if(-1 == (flags = fcntl(fd, F_GETFL, 0))){
    flags = 0;
  }

  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

//TODO: Move this function to a new source file for socket operations
int sendall(int conn_fd, uint8_t *buf, int size)
{

  int rvalue     = 0;
  int bytes2send = size;

  do{

    rvalue = send(conn_fd, buf, bytes2send, 0);
    if(-1 == rvalue){
      fprintf(stderr, "sendall failed:%s\n", strerror(errno));
      return -1;
    } else if(rvalue == 0){
      fprintf(stderr, "sendall failed:%s\n", strerror(errno));
      return -1;
    }

    bytes2send -= rvalue ;
    buf += rvalue;

  }while(bytes2send > 0);

  return 0;
}


void* socketManager(void *arg)
{
  int fdBind;
  int fdAccept;
  int len;
  int status = 0;
  struct sockaddr_un local;
  struct sockaddr_un remote;

  uint8_t interfaceBuf[maxPayloadSize];
  struct packet_s *pack2Recv = NULL;
  struct packet_s *pack2Send = NULL;
    
  struct dataGenType_s* cfg = (struct dataGenType_s *)arg;

  //pack2Send = (struct packet_s *)malloc(sizeof(struct packet_s));

  if( (fdBind = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
    fprintf(stderr, "can't open socket: %s\n", strerror(errno));
    return (void *)NULL;
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, cfg->socketPath);
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);

  if(bind(fdBind, (struct sockaddr *)&local, len) == -1){
    fprintf(stderr, "can't bind: %s\n", strerror(errno));
    return (void *)NULL;
  }

  if(-1 == listen(fdBind, 5)){  // number of listeners can actually be less
    fprintf(stderr, "can't listen: %s\n", strerror(errno));
    return (void *)NULL;  
  }


  for(;;){

    socklen_t size = sizeof(remote);
    int rcvdBytes = 0;

    fprintf(stderr,"waiting for a connection\n");

    if( -1 == (fdAccept = accept(fdBind, (struct sockaddr *)&remote, &size)) ){
      fprintf(stderr, "can't accept: %s\n", strerror(errno));
      return (void *)NULL;
    }
    setNonBlocking(fdAccept);

    pthread_mutex_lock(&(cfg->mutex));
    cfg->socketStatus = CONN_ESTABLISHED;
    pthread_mutex_unlock(&(cfg->mutex));



    do{

      // see if we can receive anydata from the socket
      rcvdBytes = recv(fdAccept, interfaceBuf, maxPayloadSize, 0);
      if( rcvdBytes > 0){ // This should actually be larger than 4 hence chunk core length

        pack2Recv = (struct packet_s *)malloc(sizeof(struct packet_s));
        fprintf(stderr, "rcvd bytes no: %d\n", rcvdBytes);
        sleep(1);
        for(int j=0; j<4; j++){
            fprintf(stderr, "buf[%d]=%d\n", j, interfaceBuf[j]);
        }
        pack2Recv->dac  = interfaceBuf[0] | (interfaceBuf[1] << 8);
        pack2Recv->len  = interfaceBuf[2] | (interfaceBuf[3] << 8);
        if(rcvdBytes > 4){
            pack2Recv->data = (uint8_t *)malloc(sizeof(uint8_t) * rcvdBytes - CHUNKCORELENGTH);
            memcpy(pack2Recv->data, &interfaceBuf[CHUNKCORELENGTH], pack2Recv->len);
        }else{
            pack2Recv->data = NULL;
        }

        fprintf(stderr, "getting some data from %s\n", cfg->socketPath);
        fprintf(stderr, "dac: %d\n", pack2Recv->dac);
        fprintf(stderr, "len: %d\n", pack2Recv->len);
        push_front(cfg->llistRx, pack2Recv);
      }
      else if( rcvdBytes < 0){
          //fprintf(stderr, "recv xx: %s\n", strerror(errno));
          
      }else if( 0 == rcvdBytes){
          fprintf(stderr, "seems like opposing side has closed the connection\n");
          break;
      }

      // see if there is any data waiting to be sent
      pack2Send = back(cfg->llistTx);
      if( (NULL == pack2Send) || is_empty(cfg->llistTx) ){
        // no data in the linked list
        fprintf(stderr, "no data to send\n");
        sleep(3);
      } else {
        // copy dac, len and memory info 
        interfaceBuf[0] = pack2Send->dac;
        interfaceBuf[1] = (pack2Send->dac & 0xFF00) >> 8;
        interfaceBuf[2] = pack2Send->len;
        interfaceBuf[3] = (pack2Send->dac & 0xFF00) >> 8;
        memcpy( &interfaceBuf[CHUNKCORELENGTH], pack2Send->data, pack2Send->len);

        if( sendall(fdAccept, interfaceBuf, pack2Send->len + CHUNKCORELENGTH) == 0){
          // data has been sent with success

          remove_back(cfg->llistTx, freeLinkedListData);
          //remove_back(cfg->llistTx, keepLinkedListData);
        } else {
          //status = -1;
          fprintf(stderr, "sendall failed\n");
        }

      } // if( (NULL == txbuf) || is_empty(cfg->llistTx) )

    }while(!status);

    fprintf(stderr, "I'll be waiting for anotherconnection\n");
    close(fdAccept);
  }
}

void freeLinkedListData(void *data)
{
  free(data);
}

void keepLinkedListData(void *data)
{
  // keep data intact
}




struct dataGenType_s** genConfigForThreads(const char** sockPaths, const int socketNo)
{

  struct dataGenType_s ** dataGenInfo = NULL;

  dataGenInfo = (struct dataGenType_s **)calloc(NOOFSOCKETS, sizeof(struct dataGenType_s *));

  fprintf(stderr, "genConfigforThreads func starts\n");

  for(int i=0; i< NOOFSOCKETS; i++){
    dataGenInfo[i] = (struct dataGenType_s *)malloc(sizeof(struct dataGenType_s));
    (dataGenInfo[i])->llistRx = create_list();
    (dataGenInfo[i])->llistTx = create_list();
    (dataGenInfo[i])->function = functionNames[i];
    (dataGenInfo[i])->socketPath = socketNames[i];
    (dataGenInfo[i])->socketStatus = CONN_WAITING;
    pthread_mutex_init(&((dataGenInfo[i])->mutex),NULL);
  }

  return dataGenInfo;
}
