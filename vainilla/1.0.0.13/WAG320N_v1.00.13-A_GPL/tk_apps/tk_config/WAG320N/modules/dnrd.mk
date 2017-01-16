include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = dnrd

CFLAGS  =   -O2 -s -Wall -I$(INC_KERNEL_PATH)
CFLAGS	+=  -DENABLE_PIDFILE -DEMBED -DHAVE_USLEEP
LIBS	+=  $(LDLIBS)

INC_KERNEL_PATH=$(TK_KERNEL_PATH)/include

