#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#define BUF_SIZE 128

static char device_buffer[BUF_SIZE];
static size_t data_len;

static int simple_misc_open(struct inode *inode, struct file *file)
{
    pr_info("simple_misc: open\n");
    return 0;
}

static int simple_release(struct inode *inode, struct file *file)
{
    pr_info("simple_misc: release\n");
    return 0;
}

static ssize_t simple_write(struct file *file,
                            const char __user *buf,
                            size_t len,
                            loff_t *ppos)
{
    if (len > BUF_SIZE - 1)
        len = BUF_SIZE - 1;

    if (copy_from_user(device_buffer, buf, len))
        return -EFAULT;

    device_buffer[len] = '\0';
    data_len = len;

    pr_info("simple_misc: write \"%s\"\n", device_buffer);

    return len;
}

static ssize_t simple_read(struct file *file,
                           char __user *buf,
                           size_t len,
                           loff_t *ppos)
{
    if (*ppos != 0)
        return 0;   /* EOF */

    if (len > data_len)
        len = data_len;

    if (copy_to_user(buf, device_buffer, len))
        return -EFAULT;

    *ppos += len;

    pr_info("simple_misc: read\n");

    return len;
}

static const struct file_operations simple_fops = {
    .owner   = THIS_MODULE,
    .open    = simple_misc_open,
    .release = simple_release,
    .read    = simple_read,
    .write   = simple_write,
};

static struct miscdevice simple_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "simple_misc",
    .fops  = &simple_fops,
};

static int __init simple_init(void)
{
    int ret;

    ret = misc_register(&simple_misc);
    if (ret)
        return ret;

    pr_info("simple_misc: loaded\n");
    return 0;
}

static void __exit simple_exit(void)
{
    misc_deregister(&simple_misc);
    pr_info("simple_misc: unloaded\n");
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Son");
MODULE_DESCRIPTION("Minimal misc character device");