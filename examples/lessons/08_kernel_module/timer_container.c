#include <linux/timer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>

#define GPIO0_BASE         0x44E07000
#define GPIO_OE            0x134
#define GPIO_SETDATAOUT    0x194
#define GPIO_CLEARDATAOUT  0x190

#define LED_PIN            (1 << 20)

struct led_dev {
    void __iomem     *gpio_base;
    int               led_state;
    struct timer_list timer;
};

static struct led_dev my_led;

/* Timer callback */
static void timer_callback(struct timer_list *t)
{
    struct led_dev *dev;

    dev = container_of(t, struct led_dev, timer);

    if (dev->led_state) {

        writel(LED_PIN,
               dev->gpio_base + GPIO_CLEARDATAOUT);

        dev->led_state = 0;

    } else {

        writel(LED_PIN,
               dev->gpio_base + GPIO_SETDATAOUT);

        dev->led_state = 1;
    }

    pr_info("LED state = %d\n", dev->led_state);

    mod_timer(&dev->timer,
              jiffies + HZ);
}

static int __init timer_init(void)
{
    uint32_t val;

    my_led.gpio_base = ioremap(GPIO0_BASE, SZ_4K);
    if (!my_led.gpio_base)
        return -ENOMEM;

    /* GPIO0_20 output */
    val = readl(my_led.gpio_base + GPIO_OE);
    val &= ~LED_PIN;
    writel(val, my_led.gpio_base + GPIO_OE);

    my_led.led_state = 0;

    timer_setup(&my_led.timer,
                timer_callback,
                0);

    mod_timer(&my_led.timer,
              jiffies + HZ);

    pr_info("Timer started\n");

    return 0;
}

static void __exit timer_exit(void)
{
    del_timer_sync(&my_led.timer);

    writel(LED_PIN,
           my_led.gpio_base + GPIO_CLEARDATAOUT);

    iounmap(my_led.gpio_base);

    pr_info("Timer stopped\n");
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Son");
MODULE_DESCRIPTION("GPIO0_20 blink using timer and container_of");