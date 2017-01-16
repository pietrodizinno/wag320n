include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

PROGRAM_NAME:= wps_monitor

WL_DRV_PATH=$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)
CMS_COMMON_LIBS   := -lcms_util -lcms_boardctl -lcrypt -lutil

LDFLAGS = -Os
LDFLAGS += -L$(EXTRALIBDIR)
LDFLAGS += -lc
LDFLAGS += -lgcc_s
LDFLAGS += -L$(BSP_PROFILE_DIR)/lib
LDFLAGS += -L$(TK_APPS_PATH)/libs/libnv
LDFLAGS += -lnv_bcm
LDFLAGS += -L$(WL_DRV_PATH)/bcmcrypto
LDFLAGS += -lwlbcmcrypto
LDFLAGS += -L$(WL_DRV_PATH)/router/shared
LDFLAGS += -lwlbcmshared
LDFLAGS += -L$(TK_APPS_PATH)/wlctl
LDFLAGS += -lwlctl
LDFALSG += -ldl
LDFLAGS += -L$(BSP_PROFILE_DIR)/lib/public
LDFLAGS += -L$(BSP_PROFILE_DIR)/lib/private
LDFLAGS += -lcms_msg
LDFLAGS += $(CMS_COMMON_LIBS)
LDFLAGS += -L.
LDFLAGS += -lwps
LDFLAGS += -L$(TK_APPS_PATH)/../target/lib

CFLAGS += -I.
CFLAGS += -I./common/include
CFLAGS += -I$(WL_DRV_PATH)/common/include
CFLAGS += -I$(WL_DRV_PATH)/include
CFLAGS += -I$(WL_DRV_PATH)/include/bcmcrypto
CFLAGS += -I$(WL_DRV_PATH)/router/bcmupnp/include
CFLAGS += -I$(WL_DRV_PATH)/router/bcmupnp/device
CFLAGS += -I$(WL_DRV_PATH)/router/bcmupnp/device/WFADevice
CFLAGS += -I$(WL_DRV_PATH)/router/eapd
CFLAGS += -I$(WL_DRV_PATH)/router/nas
CFLAGS += -I$(WL_DRV_PATH)/router/shared
CFLAGS += -I$(WL_DRV_PATH)/shared
CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
CFLAGS += -I$(BSP_DIR)/userspace/private/include
CFLAGS += -I$(BSP_DIR)/userspace/public/include
CFLAGS += -I$(BSP_DIR)/userspace/public/include/linux
CFLAGS += -DBCMWPA2
CFLAGS += -DDSLCPE
CFLAGS += -D_REENTRANT 
CFLAGS += -D__linux__
CFLAGS += -DIL_BIGENDIAN
# CFLAGS += -D_TUDEBUGTRACE

vpath %.c ./brcm_apps/apps
vpath %.c ./brcm_apps/wl
vpath %.c ./linux/ap
vpath %.c ./common/ap
vpath %.c ./common/enrollee
vpath %.c ./common/registrar
vpath %.c ./common/shared
vpath %.c ./common/sta

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib
