include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = IGMPProxy
export OBJ = igmp_proxy.o listlib.o
export SLIB = 
export DLIB = 

export CFLAGS = -Os -Wall 
export LDFLAGS = 
export ARFLAGS =
export STFLAGS =

export INC = 
export KER_INC =
export LDLIBS = 

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
