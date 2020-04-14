KERNELDIR := /home/xieli/linux/linux_xli
CURRENT_PATH := $(shell pwd)
obj-m := atomic.o

build: kernel_modules

# make modules:读取源码并将其编译为.ko文件
kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

print:
	@echo make = $(MAKE)