#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

int main( int argc, char *argv[] )
{
    int s;
    struct ifreq buffer;

    s = socket(PF_INET, SOCK_DGRAM, 0);

    memset(&buffer, 0x00, sizeof(buffer));

    strcpy(buffer.ifr_name, "eth0");

    ioctl(s, SIOCGIFHWADDR, &buffer);

    close(s);

    for( s = 0; s < 6; s++ )
    {
        printf("%.2X ", (unsigned char)buffer.ifr_hwaddr.sa_data[s]);
    }

    printf("\n");

    return 0;
}

