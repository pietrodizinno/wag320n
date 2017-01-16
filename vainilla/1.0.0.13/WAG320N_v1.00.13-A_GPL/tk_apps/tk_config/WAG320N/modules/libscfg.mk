include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = 
export OBJ = socket_tools.o
export SLIB = 
export DLIB = libscfg.so

export CFLAGS = -Os -Wall -fPIC -I$(TK_APPS_PATH)/libs/libnv
export LDFLAGS = -shared -Wl,-soname,libscfg.so
export ARFLAGS =
export STFLAGS =

export INC = #-I$(TK_APPS_PATH)/libs/libnv
export KER_INC =
export LDLIBS = 

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/lib
