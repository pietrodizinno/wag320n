

export TK_KERNEL_PATH := $(TK_APPS_PATH)/../kernel/src
export TK_INSTALL_PATH := $(TK_APPS_PATH)/../target
export PATH := /opt/toolchains/uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin:$(PATH)
#export PATH := /opt/toolchains/uclibc-crosstools-gcc-4.2.3-3/usr/bin:$(PATH)


export MAKE := $(shell which make)
export ARCH := mips
export TARGET_PREFIX := mips-linux-uclibc-
export CROSS_COMPILE := mips-linux-uclibc-


###################################################################################################
export CC := $(CROSS_COMPILE)gcc
export AR := $(CROSS_COMPILE)ar
export AS := $(CROSS_COMPILE)as
export LD := $(CROSS_COMPILE)ld
export NM := $(CROSS_COMPILE)nm
export CPP := $(CROSS_COMPILE)g++
export READELF := $(CROSS_COMPILE)readelf
export RANLIB := $(CROSS_COMPILE)ranlib
export STRIP := $(CROSS_COMPILE)strip
export SIZE := $(CROSS_COMPILE)size
export OBJCOPY := $(CROSS_COMPILE)objcopy
export OBJDUMP := $(CROSS_COMPILE)objdump
export TARGET_PREFIX := mips-linux-uclibc

