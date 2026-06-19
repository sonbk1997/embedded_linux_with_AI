#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/timer.h>

#define GPIO0_BASE    0x44E07000
#define GPIO_OE       0x134
#define GPIO_DATAOUT  0x13C

struct led_dev {
    void __iomem        *gpio_base;
    u32                 pin_mask;
    int                 led_state;
    int                 led_blink_interval;
    bool                blinking;
    spinlock_t          lock;
    struct timer_list   timer;
    struct miscdevice   miscdev;
};

// For IOCTL Control
#define LED_MAGIC 'L'
#define LED_SET_STATE           _IOW(LED_MAGIC, 0, int)
#define LED_GET_STATE           _IOR(LED_MAGIC, 1, int)
#define LED_TOGGLE_STATE        _IO(LED_MAGIC, 2)
#define LED_BLINK_STATE         _IOW(LED_MAGIC, 3, int)
#define LED_SET_BLINK_INTERVAL  _IOW(LED_MAGIC, 4, int)
#define LED_GET_BLINK_INTERVAL  _IOR(LED_MAGIC, 5, int)

static struct led_dev led_green = {
    .pin_mask = (1u << 20),
    .led_blink_interval = 1000,
};

static struct led_dev led_red = {
    .pin_mask = (1u << 31),
    .led_blink_interval = 1000,
};

static void led_hw_set(struct led_dev *dev,
                       int state)
{
    unsigned long flags;
    spin_lock_irqsave(&dev->lock, flags);
    if(state) {
        writel(readl(dev->gpio_base + GPIO_DATAOUT) | dev->pin_mask, dev->gpio_base + GPIO_DATAOUT);
    } else {
        writel(readl(dev->gpio_base + GPIO_DATAOUT) & ~dev->pin_mask, dev->gpio_base + GPIO_DATAOUT);
    }

    dev->led_state = state;
    spin_unlock_irqrestore(&dev->lock, flags);
}

static long led_ioctl(struct file *file,
                         unsigned int cmd,
                         unsigned long arg)
{
    struct led_dev *dev = file->private_data;
    switch (cmd)
    {
    case LED_SET_STATE:
    {
        if (copy_from_user(&dev->led_state, (int __user *)arg, sizeof(dev->led_state)))
            return -EFAULT;
        led_hw_set(dev, dev->led_state);
        pr_info("simple_ioctl: LED_SET_STATE command received, state=%d\n", dev->led_state);
        break;
    }
    case LED_GET_STATE:
    {
        // Read the current LED state from hardware to ensure we return the actual state    
        u32 reg;
        reg = readl(dev->gpio_base + GPIO_DATAOUT);
        dev->led_state = !!(reg & dev->pin_mask);
        if (copy_to_user((int __user *)arg, &dev->led_state, sizeof(dev->led_state)))
            return -EFAULT;
        pr_info("simple_ioctl: LED_GET_STATE command received, state=%d\n", dev->led_state);
        break;
    }
    case LED_TOGGLE_STATE:
        dev->led_state = !dev->led_state;
        led_hw_set(dev, dev->led_state);
        pr_info("simple_ioctl: LED_TOGGLE_STATE command received, state=%d\n", dev->led_state);
        break;

    case LED_BLINK_STATE:
    {
        if (copy_from_user(&dev->blinking, (int __user *)arg, sizeof(dev->blinking)))
            return -EFAULT;
        if(dev->blinking) {
            pr_info("simple_ioctl: LED_BLINK_STATE command received enable \n");
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->led_blink_interval));
        } else {
            pr_info("simple_ioctl: LED_BLINK_STATE command received disable\n");
            del_timer_sync(&dev->timer);
        }
        break;
    }

    case LED_SET_BLINK_INTERVAL:
    {
        // Implementation for setting blink interval
        pr_info("simple_ioctl: LED_SET_BLINK_INTERVAL command received\n");
        del_timer_sync(&dev->timer);
        if (copy_from_user(&dev->led_blink_interval, (int __user *)arg, sizeof(dev->led_blink_interval)))
            return -EFAULT;
        if (dev->led_blink_interval <= 0 || dev->led_blink_interval > 10000) {
            pr_info("simple_ioctl: Invalid blink interval %d, must be > 0 and < 10000\n", dev->led_blink_interval);
            return -EINVAL;
        }
        // Update the timer with the new blink interval
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->led_blink_interval));
        break;
    }
    case LED_GET_BLINK_INTERVAL:
    {
        // Implementation for getting blink interval
        pr_info("simple_ioctl: LED_GET_BLINK_INTERVAL command received\n");
        if (copy_to_user((int __user *)arg, &dev->led_blink_interval, sizeof(dev->led_blink_interval)))
            return -EFAULT;
        break;
    }
    default:
        pr_info("led_ioctl: unknown cmd=0x%x\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static void timer_callback(struct timer_list *t)
{
    struct led_dev *dev = container_of(t, struct led_dev, timer);
    if(!dev->blinking) {
        return;
    }
    dev->led_state = !dev->led_state;
    led_hw_set(dev, dev->led_state);

    pr_info("LED state toggled to %d\n", dev->led_state);

    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->led_blink_interval));
}

static int led_ioctl_open(struct inode *inode,
                       struct file *file)
{
    struct miscdevice *mdev;
    struct led_dev *dev;

    mdev = file->private_data;

    dev = container_of(mdev,
                       struct led_dev,
                       miscdev);

    file->private_data = dev;

    pr_info("%s opened\n",
            dev->miscdev.name);

    return 0;
}

static int led_ioctl_release(struct inode *inode,
                          struct file *file)
{
    return 0;
}

static const struct file_operations led_fops = {
    .owner          = THIS_MODULE,
    .open           = led_ioctl_open,
    .release        = led_ioctl_release,
    .unlocked_ioctl = led_ioctl,
};

static int __init led_ioctl_init(void)
{
    int ret;
    led_green.led_blink_interval = 1000; // Default blink interval 1000 ms
    led_red.led_blink_interval      = 1000; // Default blink interval 1000 ms

    led_green.miscdev.minor = MISC_DYNAMIC_MINOR;
    led_green.miscdev.name  = "led_green";
    led_green.miscdev.fops  = &led_fops;

    led_red.miscdev.minor = MISC_DYNAMIC_MINOR;
    led_red.miscdev.name  = "led_red";
    led_red.miscdev.fops  = &led_fops;
    spin_lock_init(&led_green.lock);
    spin_lock_init(&led_red.lock);

    led_green.gpio_base = ioremap(GPIO0_BASE, SZ_4K);
    if (!led_green.gpio_base) {
        pr_err("led_ioctl: failed to ioremap GPIO0\n");
        return -ENOMEM;
    }
    led_red.gpio_base = led_green.gpio_base; // Assuming both LEDs are on the same GPIO bank

    /* Configure GPIO pin as output */
    u32 reg;
    reg = readl(led_green.gpio_base + GPIO_OE);
    reg &= ~led_green.pin_mask;
    reg &= ~led_red.pin_mask;
    writel(reg,
        led_green.gpio_base + GPIO_OE);

    ret = misc_register(&led_green.miscdev);
    if (ret) {
        iounmap(led_green.gpio_base);
        return ret;
    }
    ret = misc_register(&led_red.miscdev);
    if (ret)
    {
        misc_deregister(&led_green.miscdev);
        iounmap(led_green.gpio_base);
        return ret;
    }

    // Timer setup for blinking
    timer_setup(&led_green.timer, timer_callback, 0);
    timer_setup(&led_red.timer, timer_callback, 0);

    pr_info("led_ioctl: loaded\n");

    return 0;
}

static void __exit led_ioctl_exit(void)
{
    // Delete the timer to stop blinking
    del_timer_sync(&led_green.timer);
    del_timer_sync(&led_red.timer);

    /* Restore GPIO pin as input */
    writel(readl(led_green.gpio_base + GPIO_OE) | led_green.pin_mask, led_green.gpio_base + GPIO_OE);
    writel(readl(led_red.gpio_base + GPIO_OE) | led_red.pin_mask, led_red.gpio_base + GPIO_OE);

    misc_deregister(&led_green.miscdev);
    misc_deregister(&led_red.miscdev);
    iounmap(led_green.gpio_base);
    pr_info("led_ioctl: unloaded\n");
}

module_init(led_ioctl_init);
module_exit(led_ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SonLH");
MODULE_DESCRIPTION("Simple IOCTL driver for controlling an LED via GPIO");