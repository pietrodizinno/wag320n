include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

PROGRAM_NAME:= ebtables

TOOLCHAIN=old
ifneq ($(strip $(TOOLCHAIN)),old)
BRCM_COMMON_CFLAGS :=  -Os -march=mips32  -fomit-frame-pointer -fno-strict-aliasing -mabi=32 -G 0 -msoft-float -pipe -Wa,-mips32
BRCM_APP_CFLAGS =  $(BRCM_COMMON_CFLAGS) -mno-shared
BRCM_SO_CFLAGS =   $(BRCM_COMMON_CFLAGS)
else
BRCM_COMMON_CFLAGS := -Wall -Dmips -G 0 -g -Os -fomit-frame-pointer -fno-strict-aliasing -fno-exceptions
export BRCM_APP_CFLAGS :=
export BRCM_SO_CFLAGS :=
endif

PROGNAME:=ebtables
PROGVERSION:=2.0.6
PROGDATE:=November\ 2003

#CFLAGS:=-Wall -Wunused
CFLAGS:=-Wall -Wunused -Os -s
CFLAGS += $(BRCM_APP_CFLAGS)

ifeq ($(shell uname -m),sparc64)
CFLAGS+=-DEBT_MIN_ALIGN=8 -DKERNEL_64_USERSPACE_32
endif

include extensions/Makefile

OBJECTS:=getethertype.o ebtables.o communication.o $(EXT_OBJS)

KERNEL_INCLUDES?=$(TK_KERNEL_PATH)/include


PROGSPECS:=-DPROGVERSION=\"$(PROGVERSION)\" \
	-DPROGNAME=\"$(PROGNAME)\" \
	-DPROGDATE=\"$(PROGDATE)\" 

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib


