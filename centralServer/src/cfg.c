#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pack.h"
#include "cJSON.h"
#include "cfg.h"
#include "msgformat.h"


/*
Sample configuration JSON and its interpretation
{
    "id": "EagleEye", 
    "format": {
        "type":       "hd", 
        "width":      1920, 
        "height":     1080, 
        "interlace":  false, 
        "frame rate": 24
    }
}

*/


struct cfg_s G_jsonCfg;
cJSON *root = NULL;



void printCfg(struct cfg_s *cfg)
{
  printf("id: %s\n", cfg->id);
  printf("type: %d\n", cfg->type);
  printf("width: %d\n", cfg->width);
  printf("height: %d\n", cfg->height);
  printf("interlace: %d\n", cfg->interlace);
  printf("frameRate: %d\n", cfg->fps);

}

// note that since the configuration file is huge
// we'll be needing multiple functions for modifying
// in order to save bandwidth

int prepCfgStruct(void)
{

  /*
   *  implement this open section according to links hence implement memory mapped access
   *  https://www.cs.purdue.edu/homes/fahmy/cs503/mmap.txt
   *  http://www.linuxquestions.org/questions/programming-9/mmap-tutorial-c-c-511265/ 
   */

  FILE *f = fopen("../configFile/config.json","rb");
  fseek(f, 0, SEEK_END);
  long len=ftell(f);
  fseek(f, 0, SEEK_SET);
  fprintf(stderr, "dbg: %d\n", len);

  char *data = (char *)malloc(len + 1);
  fread(data, 1, len, f);
  fclose(f);

  //cJSON *root   = cJSON_Parse(data);
  root   = cJSON_Parse(data);
  cJSON *format = cJSON_GetObjectItem(root, "format");

  //snprintf((char *)(G_jsonCfg.id), 25, cJSON_GetObjectItem(root, "id")->valuestring);
  strncpy((char *)(G_jsonCfg.id),(const char *)cJSON_GetObjectItem(root, "id")->valuestring, 25);

  G_jsonCfg.type       = cJSON_GetObjectItem(format, "type")->valuedouble;
  G_jsonCfg.width      = cJSON_GetObjectItem(format, "width")->valuedouble;
  G_jsonCfg.height     = cJSON_GetObjectItem(format, "height")->valuedouble;
  G_jsonCfg.interlace  = cJSON_GetObjectItem(format, "interlace")->valuedouble;
  G_jsonCfg.fps        = cJSON_GetObjectItem(format, "framerate")->valuedouble;
  
  printf("id: %s\n", G_jsonCfg.id);
  printf("width: %d\n", G_jsonCfg.width);
  printf("height: %d\n", G_jsonCfg.height);
  printf("type: %d\n", G_jsonCfg.type);
  printf("interlace: %d\n", G_jsonCfg.interlace);
  printf("frameRate: %d\n", G_jsonCfg.fps);

  return 0;
}


int mdfCfg(struct packet_s* pack)
//int modifyConfig(struct cfg_s* pack)
{
  FILE *f = NULL;
  struct cfg_s*  newCfg = (struct cfg_s *)(pack->data);
  
  //snprintf((char *)(G_jsonCfg.id), 25, newCfg->id);
  strncpy((char *)(G_jsonCfg.id), newCfg->id, 25);
  G_jsonCfg.type       = newCfg->type;
  G_jsonCfg.width      = newCfg->width;
  G_jsonCfg.height     = newCfg->height;
  G_jsonCfg.interlace  = newCfg->interlace;
  G_jsonCfg.fps        = newCfg->fps;
  
  char* rendered = cJSON_Print(root);
  f = fopen("./config.json", "w");
  fprintf(f,"%s", rendered);
  fclose(f);

  return 0;
}


struct cfg_s* getCfg(void)
{

  struct cfg_s* newCfg = NULL;

  newCfg = (struct cfg_s *)malloc(sizeof(struct cfg_s));
  memcpy(newCfg, &(G_jsonCfg), sizeof(struct cfg_s));

  return newCfg;
  //return genPack((uint8_t *) newCfg, DAC_CFG_RES, sizeof(struct cfg_s));
}
