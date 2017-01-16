include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = nbtscan
export OBJ = nbtscan.o statusq.o range.o list.o getMAC.o
export SLIB =
export DLIB =

export CFLAGS =
CFLAGS += -O2 -Wall
CFLAGS += -DPACKAGE_NAME=\"\"
CFLAGS += -DPACKAGE_TARNAME=\"\"
CFLAGS += -DPACKAGE_VERSION=\"\"
CFLAGS += -DPACKAGE_STRING=\"\"
CFLAGS += -DPACKAGE_BUGREPORT=\"\"
CFLAGS += -DSTDC_HEADERS=1
CFLAGS += -DHAVE_SYS_TYPES_H=1
CFLAGS += -DHAVE_SYS_STAT_H=1
CFLAGS += -DHAVE_STDLIB_H=1
CFLAGS += -DHAVE_STRING_H=1
CFLAGS += -DHAVE_MEMORY_H=1
CFLAGS += -DHAVE_STRINGS_H=1
CFLAGS += -DHAVE_INTTYPES_H=1
CFLAGS += -DHAVE_STDINT_H=1
CFLAGS += -DHAVE_UNISTD_H=1
CFLAGS += -DHAVE_SYS_TIME_H=1
CFLAGS += -DHAVE_STDINT_H=1
CFLAGS += -Dmy_uint8_t=uint8_t
CFLAGS += -Dmy_uint16_t=uint16_t
CFLAGS += -Dmy_uint32_t=uint32_t
CFLAGS += -DTIME_WITH_SYS_TIME=1
CFLAGS += -DHAVE_SNPRINTF=1
CFLAGS += -DHAVE_INET_ATON=1
CFLAGS += -DHAVE_SOCKET=1
CFLAGS += -DUNIX=1

ifeq ($(WIFI),1)
CFLAGS += -D_WIRELESS_
endif

export LDFLAGS =
export ARFLAGS =
export STFLAGS =

export INC =
export KER_INC =
export LDLIBS = -lpthread

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR =