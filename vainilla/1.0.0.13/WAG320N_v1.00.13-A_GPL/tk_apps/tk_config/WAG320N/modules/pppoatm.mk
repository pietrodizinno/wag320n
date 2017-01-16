include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = 
export OBJ = pppoatm.o
export SLIB =
export DLIB = libpppoatm.so

export CFLAGS = -Os -Wall -fPIC -D_linux_=1 -D_DISABLE_SERIAL_
export LDFLAGS = -shared -Wl,-soname,libpppoatm.so
export ARFLAGS =
export STFLAGS =

export INC = -I./ -I../
export KER_INC =
export LDLIBS =

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib
