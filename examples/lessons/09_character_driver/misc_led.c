#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/gpio.h>

#define GPIO0_BASE    0x44E07000
#define GPIO_OE       0x134
#define GPIO_DATAOUT  0x13C
#define LED_PIN       (1u << 20)

static void __iomem *gpio0_base;
static uint8_t gpio_state; /* 0 = OFF, 1 = ON */

static int misc_led_open(struct inode *inode, struct file *file)
{
    pr_info("misc_led: open\n");
    return 0;
}

static int misc_led_release(struct inode *inode, struct file *file)
{
    pr_info("misc_led: release\n");
    return 0;
}

static ssize_t misc_led_write(struct file *file,
                            const char __user *buf,
                            size_t len,
                            loff_t *ppos)
{
    char c;
    if (copy_from_user(&c, buf, 1))
        return -EFAULT;

    if(c=='1') {
        writel(readl(gpio0_base + GPIO_DATAOUT)
                | LED_PIN,
                gpio0_base + GPIO_DATAOUT);

        gpio_state = 1;
    } else if(c=='0') {
        writel(readl(gpio0_base + GPIO_DATAOUT)
                & ~LED_PIN,
                gpio0_base + GPIO_DATAOUT);

        gpio_state = 0;
    } else {
        pr_info("misc_led: invalid input '%c', expected '1' or '0'\n", c);
        return -EINVAL;
    }

    pr_info("misc_led: write \"%c\"\n", c);

    return len;
}

static ssize_t misc_led_read(struct file *file,
                           char __user *buf,
                           size_t len,
                           loff_t *ppos)
{
    char value;

    if (*ppos != 0)
        return 0;   /* EOF */

    value = gpio_state ? '1' : '0';

    if (copy_to_user(buf, &value, 1))
        return -EFAULT;

    *ppos = 1;

    pr_info("misc_led: read\n");

    return 1;
}

static const struct file_operations misc_led_fops = {
    .owner   = THIS_MODULE,
    .open    = misc_led_open,
    .release = misc_led_release,
    .read    = misc_led_read,
    .write   = misc_led_write,
};

static struct miscdevice misc_led = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "misc_led",
    .fops  = &misc_led_fops,
};

static int __init misc_led_init(void)
{
	uint32_t val;
    int ret;

    /* Map GPIO0 */
    gpio0_base = ioremap(GPIO0_BASE, SZ_4K);
    if (!gpio0_base)
        return -ENOMEM;

    /* Cấu hình GPIO0_20 là output (LED) */
    val = readl(gpio0_base + GPIO_OE);
    val &= ~LED_PIN;
    writel(val, gpio0_base + GPIO_OE);

    ret = misc_register(&misc_led);
    if (ret)
        return ret;

    pr_info("misc_led: loaded\n");
    return 0;
}

static void __exit misc_led_exit(void)
{
    misc_deregister(&misc_led);
    iounmap(gpio0_base);
    pr_info("misc_led: unloaded\n");
}

module_init(misc_led_init);
module_exit(misc_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Son");
MODULE_DESCRIPTION("Misc control LED");