#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define GPIO0_BASE    0x44E07000
#define GPIO_OE       0x134
#define GPIO_DATAOUT  0x13C
#define LED_PIN       (1u << 20)
#define BUTTON_GPIO   639 // GPIO0_31 corresponds to GPIO number 639 on BeagleBone Black

static void __iomem *gpio0_base;
static int button_irq;

static irqreturn_t button_handler(int irq, void *dev_id)
{
    uint32_t val;

    /* Toggle LED */
    val = readl(gpio0_base + GPIO_DATAOUT);
    val ^= LED_PIN;
    writel(val, gpio0_base + GPIO_DATAOUT);

    printk(KERN_INFO "Button pressed — LED toggled\n");
    return IRQ_HANDLED;
}

static int __init btn_led_init(void)
{
    uint32_t val;
    int ret;

    /* Map GPIO0 */
    gpio0_base = ioremap(GPIO0_BASE, SZ_4K);
    if (!gpio0_base)
        return -ENOMEM;

    /* Cấu hình GPIO0_20 là output (LED) và GPIO0_31 là input (button) */
    val = readl(gpio0_base + GPIO_OE);
    val &= ~LED_PIN;
    writel(val, gpio0_base + GPIO_OE);

    /* Lấy IRQ number từ GPIO number */
    button_irq = gpio_to_irq(BUTTON_GPIO);
    if (button_irq < 0) {
        printk(KERN_ERR "Failed to get IRQ for GPIO%d\n", BUTTON_GPIO);
        iounmap(gpio0_base);
        return button_irq;
    }

    /* Đăng ký interrupt handler */
    ret = request_irq(button_irq,
                      button_handler,
                      IRQF_TRIGGER_FALLING,
                      "button_irq",
                      NULL);
    if (ret) {
        printk(KERN_ERR "Failed to request IRQ %d\n", button_irq);
        iounmap(gpio0_base);
        return ret;
    }

    printk(KERN_INFO "Button-LED module loaded. IRQ=%d\n", button_irq);
    return 0;
}

static void __exit btn_led_exit(void)
{
    free_irq(button_irq, NULL);
    iounmap(gpio0_base);
    printk(KERN_INFO "Button-LED module unloaded\n");
}

module_init(btn_led_init);
module_exit(btn_led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luu An Phu - Vinalinux");