#ifndef PACK_H
#define PACK_H

struct packet_s{
  uint16_t dac;
  uint16_t len;
  uint8_t* data;
};

#define CHUNKCORELENGTH 4

// function prototypes
struct packet_s* genPack(uint8_t* data, uint16_t type, uint16_t len);
int delPack(struct packet_s* pack);
void clearPack(struct packet_s* pack);
size_t packLen(struct packet_s* pack);




#endif
