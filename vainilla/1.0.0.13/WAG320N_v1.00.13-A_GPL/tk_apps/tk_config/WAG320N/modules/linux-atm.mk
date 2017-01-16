include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1


export SUBDIRS = 
SUBDIRS += src/lib
SUBDIRS += src/switch
SUBDIRS += src/arpd

export BIN =
BIN += src/arpd/.libs/atmarp
BIN += src/arpd/.libs/atmarpd

export OBJ =
export SLIB =
export DLIB = src/lib/.libs/libatm.so*

export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib
