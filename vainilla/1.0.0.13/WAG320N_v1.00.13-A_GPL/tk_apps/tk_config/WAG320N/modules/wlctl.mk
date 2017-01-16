include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1
BUILD_WLCTL = dynamic
BUILD_WLCTL_SHLIB = 1
RUN_NOISE = 0

PROGRAM_NAME:= wlctl

CFLAGS =-s -Os -fomit-frame-pointer -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
LDFLAGS=-Os -Wl,-allow-shlib-undefined
CFLAGS_EXTRA =

WIRELESS_DRIVER_PATH=$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)
INC_WIRELESS_DRIVER_PATH=$(WIRELESS_DRIVER_PATH)/include
SHARED_WIRELESS_DRIVER_PATH=$(WIRELESS_DRIVER_PATH)/shared

INC_KERNEL_INC_PATH=$(TK_KERNEL_PATH)/include
CFLAGS += -Wall -I$(INC_WIRELESS_DRIVER_PATH) -I$(INC_KERNEL_INC_PATH) -I$(INC_WIRELESS_DRIVER_PATH)/proto -I$(INC_BRCMCFM_PATH) -DDSLCPE

ifeq ($(strip $(BUILD_WLCTL)), static)
CFLAGS += -DBUILD_STATIC 
endif
CFLAGS += -DBCMWPA2 -DIL_BIGENDIAN

ifeq ($(DSLCPE_DT_BUILD),1)
CFLAGS	+= -DRADIUS_RESTRICTION
endif

ifeq ($(strip $(BRCM_CHIP)),6338)
CFLAGS += -DBCMSDIO
endif

ifeq ($(strip $(BUILD_WLCTL_SHLIB)),1) 
CFLAGS += -DDSLCPE_SHLIB
CFLAGS_EXTRA = -fpic
endif

ifneq ($(strip $(RUN_NOISE)),0)
   CFLAGS += -DDSLCPE_VERBOSE
endif

CFLAGS += -DDSLCPE

WLCTLOBJS = wlu.o wlu_cmd.o wlu_iov.o
SHAREDOBJS = bcmutils.o bcmwifi.o

ifeq ($(strip $(BUILD_WLCTL_SHLIB)),1) 
MAIN = wlu_linux_dslcpe
WLCTLOBJS += wlu_linux.o
else
MAIN = wlu_linux
endif

MAINOBJ = $(MAIN).obj
OBJS = $(WLCTLOBJS) $(SHAREDOBJS)

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib
