#ifndef SENSORNODE_H
#define SENSORNODE_D

// Public Function Prototypes
void record_a_set_of_wsn_data(void *ptr);
void * prep_a_set_of_wsn_data(void);
int calc_size_for_unrolled_data(void *ptr);
int calc_size_for_unrolled_lcd_data(void *ptr);
void * unroll_wsn_data(void *ptr);
void * unroll_wsn_data_lcd(void *ptr);
int initiate_wsn(void);
int dismiss_wsn(void);
void print_cloud_data(uint8_t *buf);


#endif
