#ifndef TEST_IOCTL_LED_H
#define TEST_IOCTL_LED_H

#include <sys/ioctl.h>

#define LED_MAGIC 'L'

#define LED_SET_STATE           _IOW(LED_MAGIC, 0, int)
#define LED_GET_STATE           _IOR(LED_MAGIC, 1, int)
#define LED_TOGGLE_STATE        _IO(LED_MAGIC, 2)
#define LED_BLINK_STATE         _IOW(LED_MAGIC, 3, int)
#define LED_SET_BLINK_INTERVAL  _IOW(LED_MAGIC, 4, int)
#define LED_GET_BLINK_INTERVAL  _IOR(LED_MAGIC, 5, int)

#endif