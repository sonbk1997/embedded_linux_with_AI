#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#define SIMPLE_MAGIC 'S'

/* Command không truyền dữ liệu */
#define SIMPLE_CMD_HELLO _IO(SIMPLE_MAGIC, 0)

static long simple_ioctl(struct file *file,
                         unsigned int cmd,
                         unsigned long arg)
{
    switch (cmd)
    {
    case SIMPLE_CMD_HELLO:
        pr_info("simple_ioctl: HELLO command received\n");
        break;

    default:
        pr_info("simple_ioctl: unknown cmd=0x%x\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static int simple_ioctl_open(struct inode *inode,
                       struct file *file)
{
    pr_info("simple_ioctl: open\n");
    return 0;
}

static int simple_release(struct inode *inode,
                          struct file *file)
{
    pr_info("simple_ioctl: release\n");
    return 0;
}

static const struct file_operations simple_fops = {
    .owner          = THIS_MODULE,
    .open           = simple_ioctl_open,
    .release        = simple_release,
    .unlocked_ioctl = simple_ioctl,
};

static struct miscdevice simple_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "simple_ioctl",
    .fops  = &simple_fops,
};

static int __init simple_init(void)
{
    int ret;

    ret = misc_register(&simple_misc);
    if (ret)
        return ret;

    pr_info("simple_ioctl: loaded\n");

    return 0;
}

static void __exit simple_exit(void)
{
    misc_deregister(&simple_misc);

    pr_info("simple_ioctl: unloaded\n");
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Son");
MODULE_DESCRIPTION("Simple ioctl example");