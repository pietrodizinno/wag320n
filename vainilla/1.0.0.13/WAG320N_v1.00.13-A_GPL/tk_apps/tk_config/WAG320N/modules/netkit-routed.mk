include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN1 = routed/routed
export BIN2 = ripquery/ripquery
export OBJ1 = 
OBJ1 += routed/af.o
OBJ1 += routed/if.o 
OBJ1 += routed/input.o 
OBJ1 += routed/main.o 
OBJ1 += routed/output.o 
OBJ1 += routed/startup.o 
OBJ1 += routed/tables.o 
OBJ1 += routed/timer.o 
OBJ1 += routed/trace.o 
OBJ1 += routed/inet.o
export OBJ2 = ripquery/query.o
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
export INSTALL_LIB_DIR =