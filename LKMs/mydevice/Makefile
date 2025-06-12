CURRENT_PATH := $(shell pwd)
LINUX_KERNEL_SRC := /lib/modules/$(shell uname -r)/build
all:
	make -C $(LINUX_KERNEL_SRC) M=$(CURRENT_PATH) modules
clean:
	make -C $(LINUX_KERNEL_SRC) M=$(CURRENT_PATH) clean