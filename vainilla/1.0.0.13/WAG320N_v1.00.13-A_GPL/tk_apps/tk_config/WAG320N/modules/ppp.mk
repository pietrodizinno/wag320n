include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1


export SUBDIRS = 
SUBDIRS += pppd/plugins/pppoe
SUBDIRS += pppd/plugins
SUBDIRS += pppd

export BIN =
BIN += pppd/pppd
BIN += pppd/plugins/pppoe/pppoefwd

export OBJ =
export SLIB =
export DLIB =
DLIB += pppd/plugins/libpppoatm.so
DLIB += pppd/plugins/pppoe/libpppoe.so

export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib
