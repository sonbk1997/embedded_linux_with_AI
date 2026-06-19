# Ghi chú thực hành: Build và chạy Kernel Module trên BeagleBone Black

**Board:** BeagleBone Black (AM335x, Cortex-A8, ARM 32-bit)
**Kernel:** 6.12.79-bone53
**Host:** Ubuntu 22.04 x86_64

---

## 1. Chuẩn bị môi trường

### 1.1 Thư mục làm việc

Tất cả tools nằm trong:

```
/home/sonlh/3.LinuxEmbedded/tools/
├── arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf/   ← cross-compiler
└── bb-kernel/                                                       ← kernel source + build scripts
    ├── build_kernel.sh
    └── KERNEL/                                                      ← kernel tree đã build
```

### 1.2 Tải và cài ARM GNU Toolchain 14.3.rel1

```bash
cd /home/sonlh/3.LinuxEmbedded/tools

# Tải toolchain
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz

# Giải nén
tar -xf arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz

# Xóa archive sau khi giải nén
rm arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz

# Kiểm tra
arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-gcc --version
```

Khi tải thủ công từ trình duyệt, vào trang **ARM Developer → GNU Toolchain Downloads**, chọn đúng:
- Host: `x86_64 Linux hosted`
- Target: `arm-none-linux-gnueabihf` (AArch32 bare-metal/Linux, hard-float)
- Version: 14.3.Rel1

> **Lưu ý quan trọng:** Kernel 6.12.79-bone53 yêu cầu GCC >= 14.x. Dùng GCC 10.x hoặc 11.x sẽ lỗi `-ftrivial-auto-var-init=zero` và `-mstack-protector-guard=tls`.

---

## 2. Build kernel từ source (bb-kernel)

### 2.1 Clone Robert Nelson's bb-kernel scripts

```bash
cd /home/sonlh/3.LinuxEmbedded/tools
git clone https://github.com/RobertCNelson/bb-kernel bb-kernel
cd bb-kernel
```

### 2.2 Chọn đúng branch cho kernel 6.12.x

```bash
git checkout am33x-v6.12
```

Branch `am33x-v6.12` = AM335x SoC + kernel 6.12.x.

### 2.3 Chạy build script

```bash
./build_kernel.sh
```

Script tự động:
- Download kernel source từ kernel.org
- Apply các patch BeagleBone-specific
- Hỏi config option (nhấn Enter để dùng default)
- Build bằng cross-compiler đã cài

**Thời gian:** 30–60 phút tùy máy host.

Sau khi hoàn tất, kernel tree nằm tại `bb-kernel/KERNEL/`.

---

## 3. Makefile cho kernel module

```makefile
KERNEL_SRC := /home/sonlh/3.LinuxEmbedded/tools/bb-kernel/KERNEL
CROSS_COMPILE := /home/sonlh/3.LinuxEmbedded/tools/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
ARCH := arm

obj-m := hello.o

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) clean
```

Ba biến quan trọng:
- `KERNEL_SRC` — trỏ đến kernel tree đã build (cần có `.config` và các header)
- `CROSS_COMPILE` — prefix của toolchain, kết thúc bằng `-` (không có `gcc`)
- `ARCH=arm` — bắt buộc cho BBB (ARM 32-bit, không phải `arm64`)

---

## 4. Build kernel module

```bash
cd /home/sonlh/3.LinuxEmbedded/embedded_linux_ai_first/examples/lessons/08_kernel_module
make
```

Output thành công:

```
make[1]: Entering directory '.../bb-kernel/KERNEL'
  CC [M]  .../hello.o
  MODPOST modules.order
  CC [M]  .../hello.mod.o
  LD [M]  .../hello.ko
make[1]: Leaving directory '.../bb-kernel/KERNEL'
```

File `.ko` tạo ra là ARM 32-bit ELF — kiểm tra:

```bash
file hello.ko
# hello.ko: ELF 32-bit LSB relocatable, ARM, EABI5 version 1 (SYSV), ...
```

---

## 5. Deploy lên BBB và chạy

### 5.1 Copy module lên board

```bash
scp hello.ko debian@192.168.137.2:/home/debian/
```

### 5.2 Load module

```bash
# Trên BBB
sudo insmod hello.ko
```

Kiểm tra output:

```bash
dmesg | tail -20
```

### 5.3 Unload module

```bash
sudo rmmod hello
dmesg | tail -5
```

---

## 6. Các lỗi đã gặp và cách giải quyết

### Lỗi 1 & 2: Flash SD card thất bại — partition table corrupt hoặc thiếu MLO

**Triệu chứng:**
- BBB in `CCCCC` liên tục trên UART, không boot được
- `fdisk -l` cho thấy start sector `1948285285` lớn hơn tổng số sector của disk
- Hoặc partition table đúng nhưng FAT partition chỉ có `ID.txt`, `START.HTM`, `sysconf.txt` — không có `MLO`

**Nguyên nhân:** `dd` pipe ghi không hoàn chỉnh, hoặc page cache của kernel không flush kịp khi ghi qua pipe.

**Fix — trình tự lệnh đúng:**
```bash
# 1. Kiểm tra partition hiện tại
sudo fdisk -l /dev/sdb

# 2. Unmount tất cả partition của SD
sudo umount /dev/sdb* 2>/dev/null || true

# 3. Xóa MBR và partition table cũ
sudo dd if=/dev/zero of=/dev/sdb bs=1M count=4 status=progress

# 4. Kiểm tra file image trước khi flash
xz --test /mnt/hgfs/Windows/am335x-debian-13.4-base-v6.12-armhf-2026-04-23-4gb.img.xz && echo "OK" || echo "FILE CORRUPT"

# 5. Flash — dùng oflag=direct để bypass page cache
xzcat /mnt/hgfs/Windows/am335x-debian-13.4-base-v6.12-armhf-2026-04-23-4gb.img.xz \
  | sudo dd of=/dev/sdb bs=4M status=progress conv=fsync oflag=direct

# 6. Kiểm tra exit code của cả hai lệnh trong pipe
echo "Exit codes: ${PIPESTATUS[@]}"   # phải là "0 0"

# 7. Re-read partition table và verify
sudo partprobe /dev/sdb
sudo fdisk -l /dev/sdb
```

> **Lưu ý:** Image nằm tại `/mnt/hgfs/Windows/` vì dùng VMware shared folder. Điều chỉnh path nếu image ở chỗ khác.
>
> `oflag=direct` là điểm mấu chốt — bypass kernel buffer cache, đảm bảo dữ liệu ghi thẳng xuống SD card thay vì bị giữ trong RAM.

---

### Lỗi 3: Login prompt không hiện trên MobaXterm

**Triệu chứng:** BBB boot đến `multi-user.target`, `serial-getty@ttyS0.service` started, nhưng nhấn Enter không thấy `login:`.

**Nguyên nhân:** MobaXterm bật hardware flow control mặc định, chặn data từ board.

**Fix:** Edit serial session → đặt **Flow control = None**, kết nối lại rồi nhấn Enter.

---

### Lỗi 4: GCC version quá cũ — không support flag mới

**Triệu chứng:**
```
error: unrecognized command-line option '-ftrivial-auto-var-init=zero'
error: unrecognized command-line option '-mstack-protector-guard=tls'
```

**Nguyên nhân:** Dùng `arm-none-linux-gnueabihf-gcc 10.3.1`, kernel 6.12 yêu cầu GCC >= 14.x.

**Fix:** Tải `arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf` từ ARM developer site, cập nhật `CROSS_COMPILE` trong Makefile.

---

### Lỗi 5: aarch64-binfmt-P — host tool không chạy được trên x86_64

**Triệu chứng:**
```
aarch64-binfmt-P: Could not open '/lib/ld-linux-aarch64.so.1': No such file or directory
```

**Nguyên nhân:** Kernel output (vmlinuz, .ko) vẫn là ARM32 cho BBB — không liên quan đến lỗi này.

Vấn đề nằm ở **host tools** trong `bb-kernel/KERNEL/scripts/` (như `modpost`, `gcc-plugins/*.so`). Đây là các binary chạy trên máy build host, không cross-compile. Nếu ai đó chạy `./build_kernel.sh` trên máy aarch64 (Mac M-series, ARM64 Linux...) thì các tool này được compile thành aarch64 binary. Khi dùng kernel tree đó trên x86_64, host tools không chạy được.

`make modules_prepare` không đủ để fix vì GCC plugins cũng là aarch64 binary và không được rebuild.

**Fix:** Chạy `modules_prepare` để rebuild host tools cho đúng kiến trúc của máy build:

```bash
cd /home/sonlh/3.LinuxEmbedded/tools/bb-kernel/KERNEL
make ARCH=arm CROSS_COMPILE=<toolchain-prefix> modules_prepare
```

Lệnh này rebuild `scripts/modpost`, `scripts/gcc-plugins/` và các host tool khác cho x86_64 mà không cần build lại toàn bộ kernel.

---

## 7. Tóm tắt toolchain cho BBB

| Mục đích | Toolchain | Path |
|---|---|---|
| Kernel module | `arm-none-linux-gnueabihf-` GCC 14.3 | `tools/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf/` |

> **BBB là ARM 32-bit (Cortex-A8) — luôn dùng `ARCH=arm`, không phải `ARCH=arm64`.**
> `aarch64` xuất hiện trong log chỉ là do build host tool issue, không liên quan đến target.
