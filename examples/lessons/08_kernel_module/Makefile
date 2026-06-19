KERNEL_SRC := /home/sonlh/3.LinuxEmbedded/tools/bb-kernel/KERNEL
CROSS_COMPILE := /home/sonlh/3.LinuxEmbedded/tools/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
ARCH := arm

obj-m := timer_container.o

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) clean
