include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = lld2
export ETC := lld2d.conf WAG320N.ico WAG320N.large.ico
export OBJ = 
OBJ += main.o
OBJ += event.o
OBJ += util.o
OBJ += packetio.o
OBJ += band.o
OBJ += state.o
OBJ += sessionmgr.o
OBJ += enumeration.o
OBJ += mapping.o
OBJ += seeslist.o
OBJ += tlv.o
OBJ += qospktio.o
OBJ += osl-linux.o
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
export INSTALL_ETC_DIR := $(TK_INSTALL_PATH)/etc
