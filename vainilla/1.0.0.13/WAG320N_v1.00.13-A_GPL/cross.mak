
CROSS_COMPILE  = mips-linux-uclibc-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
# CONFIG_MIPS_BRCM Begin Broadcom changed code.
CPP		= $(CROSS_COMPILE)g++
# CPP           = $(CC) -E
# CONFIG_MIPS_BRCM End Broadcom changed code.
AR		= $(CROSS_COMPILE)ar
RANLIB          = $(CROSS_COMPILE)ranlib
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump 
TARGET_PREFIX	= mips-linux-uclibc

export AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP TARGET_PREFIX
