#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "simple_ioctl.h"

int main(void)
{
    int fd;

    fd = open("/dev/simple_ioctl", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    printf("Send HELLO ioctl\n");

    ioctl(fd, SIMPLE_CMD_HELLO);

    close(fd);

    return 0;
}