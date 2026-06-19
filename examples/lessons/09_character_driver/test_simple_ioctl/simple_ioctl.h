#ifndef SIMPLE_IOCTL_H
#define SIMPLE_IOCTL_H

#include <sys/ioctl.h>

#define SIMPLE_MAGIC 'S'

#define SIMPLE_CMD_HELLO \
        _IO(SIMPLE_MAGIC, 0)

#endif