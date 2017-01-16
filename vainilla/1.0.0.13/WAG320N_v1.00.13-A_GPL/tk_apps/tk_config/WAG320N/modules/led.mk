include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/kernel_compile.mk

export KVER = 2.6
export STRIP_ENABLE = 0

export KERNEL_DIR = $(TK_KERNEL_PATH)

ifeq ($(KVER), 2.4)
	export CFLAGS = -D__KERNEL__ -DMODULE -D__linux__ -DMODVERSIONS
	CFLAGS += -nostdinc -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -pipe -mapcs-32 -D__LINUX_ARM_ARCH__=4 -march=armv4
	CFLAGS += -fno-common -mtune=arm9tdmi -Uarm -mshort-load-bytes -msoft-float
	CFLAGS += -Wall -Os
	CFLAGS += -D__CONFIG_MODULE_ID__=$(__MODULE_INDEX__)
	export INC = -I/usr/local/arm/2.95.3/lib/gcc-lib/arm-linux/2.95.3/include
	export KER_INC = -I$(KERNEL_DIR)/include -I$(KERNEL_DIR)/include/asm/gcc
	export KER_MOD = led.o
else ifeq ($(KVER), 2.6)
	export obj-m = led.o
endif

export STFLAGS = --strip-debug

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/lib/modules

