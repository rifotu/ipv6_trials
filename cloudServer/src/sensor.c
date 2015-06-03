typedef struct raw_node_data_s{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint16_t temp;
    uint32_t pressure;
    float humidity;
    float luminosity;
    unsigned char addr[16]; /* IPv6 address */
}raw_node_data_t;

typedef struct time_llh_s{
    int combinerID;
    int number_of_nodes;
    struct tm timestamp;
    long long latitude;
    long long longitude;
    long long height;
}time_llh_t;

