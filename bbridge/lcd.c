#include <unistd.h>
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <time.h>
#include <stdint.h>


const float zeros_13 = 10000000000.0;
const float zeros_4 = 10000.0;

#pragma pack(1)
// Structure definitions
typedef struct raw_node_data_s{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint16_t temp;
    uint32_t pressure;
    //float humidity;
    //float luminosity;
    uint16_t humidity;
    uint16_t luminosity;
    unsigned char addr[16]; /* IPv6 address */
}raw_node_data_t;

typedef struct time_llh_s{
    int combinerID;
    int number_of_nodes;
    //float latitude;
    //float longitude;
    long long latitude;
    long long longitude;
}time_llh_t;

#pragma pack()

// Public function prototypes
void print_uart_data(uint8_t *buf);
int get_payload_size(uint8_t *buf);
uint8_t * prep_uart_data(void);
void set_blocking (int fd, int should_block);
int set_interface_attribs (int fd, int speed, int parity);

// Private function prototypes
static void *prep_time_llh(void);
static void *prep_node_data(void);
static float gen_float_random(void);
static long long gen_coordinate(void);
static int gen_sensor_data(void);


static long long gen_coordinate(void)
{
    double a = 5.0;
    double ll = 0.0;

    ll = (((double)rand()/(double)(RAND_MAX)) * a);
    //printf("gen coordinate double: %f\n", ll);
    ll *= zeros_13;
    //printf("gen coordinate double: %f\n", ll);
    //printf("gen coordinate int: %lld\n", (long long)ll );
    return  (long long )ll;
}

static int gen_sensor_data(void)
{
    float a = 5.0;
    float ll = 0.0;
    ll = (((float)rand()/(float)(RAND_MAX)) * a);
    //printf("gen coordinate double: %f\n", ll);
    ll *= zeros_4;
    //printf("gen coordinate double: %f\n", ll);
    //printf("gen coordinate float: %d\n", (int)ll );
    return  (int)ll;
}
    
    


static float gen_float_random(void)
{
    float a = 5.0;
    return (((float)rand()/(float)(RAND_MAX)) * a);
}

static void *prep_node_data(void)
{
    struct raw_node_data_s *raw = NULL;

    raw = (struct raw_node_data_s *)malloc(sizeof(struct raw_node_data_s));

    raw->accel_x =  rand() % 10 - 5;
    raw->accel_y =  rand() % 20 - 10;
    raw->accel_z =  rand() % 15 - 7;
    raw->temp = rand() % 30;
    raw->pressure = rand() % 1000;
    raw->humidity = gen_sensor_data();
    raw->luminosity = gen_sensor_data();
    memcpy(raw->addr, "hello world", strlen("hello world"));

    return raw;
}

static void *prep_time_llh(void)
{
    int combinerID;
    int number_of_nodes;
    float latitude;
    float longitude;

    struct time_llh_s *time_llh = NULL;

    time_llh = (struct time_llh_s *)malloc(sizeof(struct time_llh_s));

    time_llh->combinerID = rand() % 5;
    //time_llh->number_of_nodes = rand() % 3 + 1;
    time_llh->number_of_nodes = 2;
    time_llh->latitude = gen_coordinate();
    time_llh->longitude = gen_coordinate();

    return time_llh;
}
uint8_t *prep_uart_data(void *ptr)
{
    struct time_llh_s *time_llh = NULL;
    struct raw_node_data_s *raw =  NULL;
    uint8_t *buf = NULL;
    int size = 0;
    int offset = 0;

    time_llh = pre

uint8_t * prep_uart_data(void)
{
    struct time_llh_s *time_llh = NULL;
    struct raw_node_data_s *raw = NULL;
    uint8_t *buf = NULL;
    int size = 0;
    int offset = 0;
    
    time_llh = prep_time_llh();

    size = sizeof(struct time_llh_s) + time_llh->number_of_nodes * sizeof(struct raw_node_data_s);
    buf = (uint8_t *)malloc(sizeof(uint8_t) * size);
    memcpy(buf, time_llh, sizeof(struct time_llh_s));
    free(time_llh);

    for(int i=0; i < time_llh->number_of_nodes; i++){
        offset = sizeof(struct time_llh_s) + i * sizeof(struct raw_node_data_s);
        raw = prep_node_data();
        memcpy(buf + offset, raw, sizeof(struct raw_node_data_s));
        free(raw);
    }
    
    return buf;
}

int get_payload_size(uint8_t *buf)
{
    struct time_llh_s *time_llh = (struct time_llh_s *)buf;
    int len = sizeof(struct time_llh_s) + sizeof(struct raw_node_data_s) * time_llh->number_of_nodes;
    printf("buffer length: %d\n", len);

    return len;
}

void print_uart_data(uint8_t *buf)
{
    struct time_llh_s *time_llh = NULL;
    struct raw_node_data_s *raw = NULL;
    int offset = 0;

    time_llh = (struct time_llh_s *)buf;

    printf("combiner id: %d\n", time_llh->combinerID);
    printf("number of nodes. %d\n", time_llh->number_of_nodes);
    printf("latitude: %lld\n", time_llh->latitude);
    printf("longitude: %lld\n", time_llh->longitude);

    for(int i=0; i < time_llh->number_of_nodes; i++)
    {
        offset = sizeof(struct time_llh_s) + i * sizeof(struct raw_node_data_s);
        raw = (struct raw_node_data_s *)(buf + offset);
        //printf("P:%+6ld\n", raw->pressure);
        printf("P:%zu\n", raw->pressure);
        printf("T2:%+3d\n", raw->temp);
        printf("X:%+6d | Y:%+6d | Z:%+6d\n",  raw->accel_x,
                                              raw->accel_y,
                                              raw->accel_z );
        printf("H: %d\n", raw->humidity);
        printf("L: %d\n", raw->luminosity);
    }
    
}

int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void set_blocking(int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes", errno);
}

int main(void)
{
    uint8_t *buf = NULL;  // consider having this buffer static and NOT dynamic
    int len = 0;
    //char *portname = "/dev/ttyS1";
    char *portname = "/dev/ttyO1";
    
    int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);

    // receive 25:  approx 100 uS per char transmit
    char rxbuf [100];
    srand(time(NULL));  // generate seed data for random()

    set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
    write (fd, "hello!\n", 7);           // send 7 character greeting
    usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus

    while(1){

        if (fd < 0)
        {
            printf ("error %d opening %s: %s", errno, portname, strerror (errno));
            return -1;
        }

        buf = prep_uart_data();
        len = get_payload_size(buf);
        print_uart_data(buf);

        // write to serial port
        write(fd, buf, len); // if fd is set to non blocking then sleep 
        usleep((len + 25) * 100);
        free(buf);


        int n = read (fd, rxbuf, sizeof buf);  // read up to 100 characters if ready to read
        printf("number of read characters: %d\n",n);
        sleep(1);
    }
    
    return 0;

}
