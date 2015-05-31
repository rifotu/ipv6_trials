#ifndef CLOUD_H
#define CLOUD_H

// Public function prototypes
int initiate_connection_2_cloud(void);
int kill_connection_2_cloud(void);
int send_data_2_cloud(void *buf, int size);

#endif
