#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pack.h"
#include "cfg.h"
#include "msgformat.h"



struct packet_s* genCfgChng_Notifier(void)
{
  struct packet_s* pack = NULL;
  pack = genPack(NULL, DAC_CFG_CHANGE_NOTIFIER, 0);

  return pack;
}

struct packet_s* genCfg_Req(void)
{
  struct packet_s* pack = NULL;
  pack = genPack(NULL, DAC_CFG_REQ, 0);

  return pack;
}

struct packet_s* genCfg_Res(struct cfg_s *ptr)
{
  struct packet_s* pack = NULL;
  
  pack = genPack( (uint8_t*)ptr, DAC_CFG_RES, sizeof(struct cfg_s) );

  return pack;
}

struct packet_s* genCfg_MDFY(void *ptr)
{
  struct packet_s* pack = NULL;

  pack = genPack( (uint8_t *)ptr, DAC_CFG_MDFY, sizeof(struct cfg_s) );
  
  return pack;
}

struct packet_s* genSys_Dbg(char *ptr)
{
  struct packet_s* pack = NULL;
  int len;

  len = strlen(ptr);
  pack = genPack( (uint8_t *)ptr, DAC_SYS_DBG, len);

  return pack;
}




  


