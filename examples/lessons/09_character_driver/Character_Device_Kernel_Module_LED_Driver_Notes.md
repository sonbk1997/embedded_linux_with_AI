# Embedded Linux Learning Notes
## Character Device, Kernel Module, Misc Device, IOCTL, Timer và LED Driver (BeagleBone Black)

### 1. Kernel Module là gì?
Kernel Module là đoạn code có thể nạp/rút động vào Linux Kernel mà không cần reboot.

Các lệnh:
```bash
insmod driver.ko
rmmod driver
lsmod
dmesg
```

Luồng:
```text
module_init()
    ↓
Driver hoạt động
    ↓
module_exit()
```

---

## 2. Character Device là gì?

Character Device là thiết bị được truy cập theo dạng stream byte.

Ví dụ:
- UART
- GPIO Driver
- I2C Driver
- Sensor Driver

User Space:

```c
open()
read()
write()
ioctl()
close()
```

Kernel:

```c
struct file_operations
{
    .open
    .read
    .write
    .unlocked_ioctl
    .release
};
```

---

## 3. Misc Device

Misc Device là Character Device đơn giản hóa.

Không cần:

- alloc_chrdev_region()
- cdev_init()
- cdev_add()
- class_create()
- device_create()

Chỉ cần:

```c
struct miscdevice mydev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "led_ioctl",
    .fops = &led_fops,
};

misc_register(&mydev);
```

Kernel tự tạo:

```bash
/dev/led_ioctl
```

---

## 4. File Operations

Ví dụ:

```c
static const struct file_operations led_fops = {
    .owner          = THIS_MODULE,
    .open           = led_open,
    .release        = led_release,
    .unlocked_ioctl = led_ioctl,
};
```

Mapping:

```text
User Space                 Kernel
-----------                ----------------
open()      ----------->   open()
ioctl()     ----------->   unlocked_ioctl()
close()     ----------->   release()
```

---

## 5. IOCTL là gì?

IOCTL = Input Output Control

Cho phép truyền command đặc biệt từ User Space xuống Driver.

Ví dụ:

```c
#define LED_MAGIC 'L'

#define LED_SET_STATE           _IOW(LED_MAGIC, 0, int)
#define LED_GET_STATE           _IOR(LED_MAGIC, 1, int)
#define LED_TOGGLE_STATE        _IO(LED_MAGIC, 2)
```

User Space:

```c
int state = 1;

ioctl(fd,
      LED_SET_STATE,
      &state);
```

Kernel:

```c
static long led_ioctl(struct file *file,
                      unsigned int cmd,
                      unsigned long arg)
{
    switch(cmd)
    {
    case LED_SET_STATE:
        ...
        break;
    }
}
```

---

## 6. copy_from_user() và copy_to_user()

User Space và Kernel Space tách biệt.

Không được:

```c
int state = *(int *)arg;
```

Phải dùng:

```c
copy_from_user()
copy_to_user()
```

Ví dụ:

```c
copy_from_user(&state,
               (int __user *)arg,
               sizeof(state));
```

---

## 7. Memory Mapped I/O

GPIO Register được map vào không gian địa chỉ CPU.

```c
gpio_base = ioremap(GPIO0_BASE, SZ_4K);
```

Đọc ghi:

```c
readl()
writel()
```

Ví dụ:

```c
reg = readl(base + GPIO_DATAOUT);

writel(reg | pin_mask,
       base + GPIO_DATAOUT);
```

---

## 8. Timer

Linux Kernel Timer:

```c
struct timer_list timer;
```

Khởi tạo:

```c
timer_setup(&timer,
            timer_callback,
            0);
```

Start:

```c
mod_timer(&timer,
          jiffies +
          msecs_to_jiffies(1000));
```

Stop:

```c
del_timer_sync(&timer);
```

---

## 9. container_of()

Rất quan trọng trong Linux Kernel.

Ví dụ:

```c
struct led_dev {
    int state;
    struct timer_list timer;
};
```

Trong callback chỉ nhận được:

```c
struct timer_list *t
```

Lấy lại struct cha:

```c
struct led_dev *dev;

dev = container_of(t,
                   struct led_dev,
                   timer);
```

---

## 10. file->private_data

Kỹ thuật cực kỳ phổ biến trong Driver thật.

### Trước open()

Misc framework tự gán:

```c
file->private_data
        ↓
struct miscdevice
```

### Trong open()

```c
mdev = file->private_data;

dev = container_of(mdev,
                   struct led_dev,
                   miscdev);

file->private_data = dev;
```

### Sau open()

```c
file->private_data
        ↓
struct led_dev
```

Các hàm khác:

```c
read()
write()
ioctl()
```

đều truy cập trực tiếp:

```c
struct led_dev *dev;

dev = file->private_data;
```

---

## 11. Một Driver có nhiều Misc Device

Hoàn toàn được.

Ví dụ:

```c
struct led_dev led_green;
struct led_dev led_red;
```

Mỗi LED có:

```c
struct miscdevice miscdev;
```

Kernel tạo:

```bash
/dev/led_green
/dev/led_red
```

Nhưng cùng dùng:

```c
led_fops
```

Nhờ:

```c
file->private_data
```

driver biết LED nào đang được mở.

---

## 12. Kiến trúc Driver LED cuối cùng

```text
+------------------+
| User Space App   |
+------------------+
        |
        v
ioctl()
        |
        v
+------------------+
| led_ioctl()      |
+------------------+
        |
        +------> GPIO Register
        |
        +------> Timer
```

---

## 13. Cấu trúc led_dev

```c
struct led_dev {
    void __iomem      *gpio_base;
    u32                pin_mask;

    int                led_state;
    int                led_blink_interval;

    bool               blinking;

    spinlock_t         lock;

    struct timer_list  timer;

    struct miscdevice  miscdev;
};
```

Ý nghĩa:

- gpio_base: địa chỉ GPIO
- pin_mask: chân GPIO
- led_state: trạng thái LED
- led_blink_interval: chu kỳ nhấp nháy
- blinking: đang blink hay không
- timer: timer kernel
- miscdev: tạo /dev node

---

## 14. Hai LED độc lập

```c
static struct led_dev led_green = {
    .pin_mask = (1u << 20),
};

static struct led_dev led_red = {
    .pin_mask = (1u << 31),
};
```

User Space:

```bash
/dev/led_green
/dev/led_red
```

Điều khiển độc lập.

---

## 15. Spinlock

Dùng khi nhiều context cùng truy cập dữ liệu.

Ví dụ:

- ioctl()
- timer callback

Cùng sửa:

```c
led_state
blinking
led_blink_interval
```

Khởi tạo:

```c
spin_lock_init(&dev->lock);
```

Sử dụng:

```c
unsigned long flags;

spin_lock_irqsave(&dev->lock,
                  flags);

/* critical section */

spin_unlock_irqrestore(&dev->lock,
                       flags);
```

---

## 16. Những kiến thức đã hoàn thành

- Linux Kernel Module
- Character Device
- Misc Device
- file_operations
- User Space ↔ Kernel Space
- copy_to_user()
- copy_from_user()
- IOCTL
- MMIO
- GPIO Register Programming
- Timer
- container_of()
- file->private_data
- Multi Device Driver
- Spinlock cơ bản

---

## 17. Các bước nên học tiếp

1. Wait Queue
2. Blocking Driver
3. Poll / Select
4. Interrupt Driver nâng cao
5. Platform Driver
6. Device Tree
7. GPIO Descriptor API
8. Workqueue
9. Kernel Thread
10. Real Linux Driver Architecture

---

Tác giả ghi chú: SonLH
Board: BeagleBone Black RevC
Mục tiêu: Embedded Linux Driver Development
