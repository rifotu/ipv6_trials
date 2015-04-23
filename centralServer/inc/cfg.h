#ifndef CFG_H
#define CFG_H


struct cfg_s{
  char     id[25];
  uint8_t  type;
  uint16_t width;
  uint16_t height;
  uint8_t  interlace;
  uint16_t fps;
};


int prepCfgStruct(void);
int mdfCfg(struct packet_s* pack);
void printCfg(struct cfg_s *cfg);
struct cfg_s* getCfg(void);

#endif
