include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = 
export OBJ =
export SLIB =
export DLIB =

export CFLAGS = 
export LDFLAGS =
export ARFLAGS =
export STFLAGS =

export INC =
export KER_INC =
export LDLIBS =

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR =
