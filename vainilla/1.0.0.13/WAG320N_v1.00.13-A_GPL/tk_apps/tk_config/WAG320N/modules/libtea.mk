include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = 
export OBJ = tea.o
export SLIB = 
export DLIB = libtea.so

export CFLAGS = -Os -Wall -fPIC
export LDFLAGS = -shared -Wl,-soname,libtea.so
export ARFLAGS =
export STFLAGS =

export INC = 
export KER_INC =
export LDLIBS = 

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/lib

