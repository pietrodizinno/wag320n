include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = ez-ipupdate
export SRC = 
export OBJ = ez-ipupdate.o cache_file.o
export SLIB =
export DLIB =

export CFLAGS = -DHAVE_CONFIG_H -DDYNDNS -DTZO -Os -Wall
export LDFLAGS =
export ARFLAGS =
export STFLAGS =

export INC = -I./
export KER_INC =
export LDLIBS =

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin