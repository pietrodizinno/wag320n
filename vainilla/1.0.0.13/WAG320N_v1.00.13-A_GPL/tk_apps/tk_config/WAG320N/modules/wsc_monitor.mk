include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

PROGRAM_NAME:= wsc_cms_monitor
PROGRAM_NAME2:= wsc_status

ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public
WL_DRV_PATH=$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)
CMS_COMMON_LIBS   := -lcms_util -lcms_boardctl -lcrypt -lutil
CMS_LIB_PATH = $(patsubst %,-L$(BSP_PROFILE_DIR)%,$(subst :, ,$(ALLOWED_LIB_DIRS)))


CFLAGS += -DBCMWPA2 -DIL_BIGENDIAN -DDSLCPE -DBRCM_WLAN
CFLAGS += -s -Os -Wall -fomit-frame-pointer
CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) 
CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) 
CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) 
CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
CFLAGS += -I$(WL_DRV_PATH)/include
CFLAGS += -I$(WL_DRV_PATH)/include/bcmcrypto
CFLAGS += -I$(INC_BRCMCFM_PATH) 
CFLAGS += -I$(KERNEL_DIR)/src/include

CFLAGS +=-I$(BSP_DIR)/userspace/public/include  \
         -I$(BSP_DIR)/userspace/private/include  \
		 -I$(BSP_DIR)/userspace/public/include/linux \
		 -I$(BSP_DIR)/userspace/private/libs/cms_core \
		 -I$(BSP_DIR)/userspace/private/apps/wlan/wlmngr
CXXFLAGS += $(CFLAGS)

LDFLAGS += -L$(BSP_PROFILE_DIR)/lib
LDFLAGS += -L$(WL_DRV_PATH)
LDFLAGS += -L$(TK_APPS_PATH)/libs/libnv
LDFLAGS += -L$(TK_APPS_PATH)/wlctl
LDFLAGS += -L$(BSP_DIR)/userspace/private/apps/wlan/wlmngr
BRCMLIBS = -lnv -lwlctl
BRCMLIBS += -lwlbcmshared
BRCMLIBS += -lwlbcmcrypto
BRCMLIBS += -lm
BRCMLIBS += -lpthread
BRCMLIBS += -lc

LIBS = -lcms_msg $(CMS_COMMON_LIBS) -lcms_msg $(CMS_COMMON_LIBS) -ldl

CFLAGS 	+= -Wall -Wnested-externs -D_REENTRANT -D__linux__

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
