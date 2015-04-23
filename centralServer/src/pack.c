
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pack.h"


#define PACK_CORE_LENGTH    4



/* Packet structure is defined in chunk.h for now
 *  
typedef struct packet_s{
  uint16_t dac;
  uint16_t len;
  uint8_t* data;
}packet_t;

*/



// Private function prototypes
// ....... empty for now ......





struct packet_s* genPack(uint8_t* data, uint16_t type, uint16_t len)
{
  struct packet_s* pack = NULL;

  pack = (struct packet_s*)malloc(sizeof(struct packet_s));
  pack->dac  = type;
  pack->len  = len;
  pack->data = data;

  return pack;
}


// Note that packet 
int delPack(struct packet_s* pack)
{
  int ret;
  
  // if payload hasn't been already freed, inform the user about it
  if(NULL != pack->data){
    ret = -1;
  } else {
    ret = 0;
  }

  pack->dac = 0;  // set memory space to default values before free
  pack->len = 0;
  free(pack);

  return ret;
}

void clearPack(struct packet_s* pack)
{
  if(NULL != pack->data){
    free(pack->data);
  }
  pack->dac = 0;
  pack->len = 0;
}


size_t packLen(struct packet_s* pack)
{
  size_t len = 0;

  if(NULL == pack->data){
    len = sizeof(struct packet_s);
  } else {
    len = pack->len + PACK_CORE_LENGTH;
  }

  return len;
}
