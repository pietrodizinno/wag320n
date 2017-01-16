include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = atm_monitor
export OBJ = atm_monitor.o
export SLIB =
export DLIB =

export CFLAGS = -Os -Wall
export LDFLAGS = -L$(TK_APPS_PATH)/libs/libnv
export ARFLAGS =
export STFLAGS =

export INC = -I$(TK_APPS_PATH)/libs/libnv
export KER_INC =
export LDLIBS = -lnv

export LINK := $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin


