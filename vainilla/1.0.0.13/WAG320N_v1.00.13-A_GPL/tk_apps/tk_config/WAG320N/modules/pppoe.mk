include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = pppoefwd
export OBJ1 = pppoehash.o pppoe_client.o pppoe_relay.o pppoe_server.o utils.o libpppoe.o
export OBJ2 = pppoe.o
export OBJ3 = pppoefwd.o
export SLIB =
export DLIB = libpppoe.so

export CFLAGS = -Os -Wall -fPIC -D_linux_=1 -D_DISABLE_SERIAL_
export LDFLAGS1 = -shared -Wl,-soname,libpppoe.so
export LDFLAGS2 =
export ARFLAGS =
export STFLAGS =

export INC = -I../../
export KER_INC =
export LDLIBS1 =
export LDLIBS2 =

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib
