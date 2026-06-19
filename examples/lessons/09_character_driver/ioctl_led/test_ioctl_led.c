#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "test_ioctl_led.h"

int main(void)
{
    int fd;

    fd = open("/dev/led_red", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    printf("Send LED_SET_STATE ioctl\n");

    int state = 1; // ON
    int result;
    int enable_blink = 0; // Disable blinking
    int blink_interval = 500; // 500 ms

    ioctl(fd, LED_SET_STATE, &state);
    ioctl(fd, LED_GET_STATE, &result);
    printf("LED state after setting ON: %d\n", result);
    sleep(5);

    state = 0; // OFF
    ioctl(fd, LED_SET_STATE, &state);
    ioctl(fd, LED_GET_STATE, &result);
    printf("LED state after setting OFF: %d\n", result);
    sleep(5);

    // Toggle LED state
    ioctl(fd, LED_TOGGLE_STATE);
    ioctl(fd, LED_GET_STATE, &result);
    printf("LED state after toggling: %d\n", result);
    sleep(5);

    enable_blink = 1;
    ioctl(fd, LED_BLINK_STATE, &enable_blink);
    blink_interval = 500; // 500 ms
    ioctl(fd, LED_SET_BLINK_INTERVAL, &blink_interval);
    ioctl(fd, LED_GET_BLINK_INTERVAL, &result);
    printf("LED blink interval: %d ms\n", result);
    sleep(20);
    enable_blink = 0;
    ioctl(fd, LED_BLINK_STATE, &enable_blink);
    printf("Disable LED blink \n");

    state = 0; // OFF
    ioctl(fd, LED_SET_STATE, &state);
    ioctl(fd, LED_GET_STATE, &result);
    printf("LED state after setting OFF: %d\n", result);
    sleep(10);

    enable_blink = 1;
    ioctl(fd, LED_BLINK_STATE, &enable_blink);
    blink_interval = 100; // 100 ms
    ioctl(fd, LED_SET_BLINK_INTERVAL, &blink_interval);
    ioctl(fd, LED_GET_BLINK_INTERVAL, &result);
    printf("LED blink interval: %d ms\n", result);
    sleep(20);
    enable_blink = 0;
    ioctl(fd, LED_BLINK_STATE, &enable_blink);

    close(fd);

    return 0;
}