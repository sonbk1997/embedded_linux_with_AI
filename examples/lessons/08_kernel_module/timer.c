#include <linux/timer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>

struct timer_list my_timer;

/* Callback — chạy trong softirq context, ràng buộc tương tự interrupt handler */
static void timer_callback(struct timer_list *t)
{
    printk(KERN_INFO "Timer fired!\n");

    /* Tự schedule lại sau 1 giây — tạo periodic timer */
    mod_timer(&my_timer, jiffies + HZ);
}

static int __init timer_init(void)
{
    /* Khởi tạo timer và gán callback */
    timer_setup(&my_timer, timer_callback, 0);

    /* Kích hoạt lần đầu — HZ = số jiffies trong 1 giây */
    mod_timer(&my_timer, jiffies + HZ);

    printk(KERN_INFO "Timer started\n");
    return 0;
}

static void __exit timer_exit(void)
{
    /* Hủy timer trước khi unload — bắt buộc */
    del_timer_sync(&my_timer);
    printk(KERN_INFO "Timer stopped\n");
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Training");
MODULE_DESCRIPTION("Timer example");
