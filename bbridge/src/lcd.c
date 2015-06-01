#include <unistd.h>
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <time.h>
#include <stdint.h>

#include "lcd.h"

// Global variables
static int fd_lcd_G = 0;

// Constant values
static const char *lcd_port = "/dev/ttyO1";
static const float zeros_13 = 10000000000.0;
static const float zeros_4 = 10000.0;

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

// Private function prototypes
static int set_interface_attribs (int fd, int speed, int parity);
static void set_blocking (int fd, int should_block);


static int set_interface_attribs (int fd, int speed, int parity)
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

static void set_blocking(int fd, int should_block)
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

int send_data_2_lcd(uint8_t *buf, int size)
{
    int n = 0;
    n = write(fd_lcd_G, buf, size);
    return n;
}


int initiate_lcd_comm(void)
{

    fd_lcd_G = open( lcd_port, O_RDWR | O_NOCTTY | O_SYNC);
    set_interface_attribs(fd_lcd_G, B115200, 0);
    set_blocking(fd_lcd_G, 0); // set to no blocking
    return 0;
}


int dismiss_lcd_comm(void)
{
    close(fd_lcd_G);
    return 0;
}
