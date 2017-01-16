include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN1 = syslogd
export OBJ1 = syslogd.o klogd.o
export BIN2 = klogd
export SLIB = 
export DLIB = 

export CFLAGS = -Os -Wall
ifeq ($(WIFI), 1)
	CFLAGS += -D_WIRELESS_
endif
export LDFLAGS = -L$(TK_APPS_PATH)/libs/libnv -L$(TK_APPS_PATH)/../target/lib
export ARFLAGS =
export STFLAGS =

export INC = -I$(TK_APPS_PATH)/libs/libnv
export KER_INC =
export LDLIBS = -lnv

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/sbin
