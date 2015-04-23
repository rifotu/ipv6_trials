#ifndef _MSGFORMAT_H
#define _MSGFORMAT_H


#define CONN_ESTABLISHED  0
#define CONN_LOST         1
#define CONN_WAITING      2



#define DAC_CFG_CHANGE_NOTIFIER   0
#define DAC_CFG_REQ               275
#define DAC_CFG_RES               2
#define DAC_CFG_MDFY              3
#define DAC_SYS_DBG               4

struct packet_s* genCfgChng_Notifier(void);
struct packet_s* genCfg_Req(void);
struct packet_s* genCfg_Res(struct cfg_s *ptr);
struct packet_s* genCfg_MDFY(void *ptr);
struct packet_s* genSys_Dbg(char *ptr);

#endif
