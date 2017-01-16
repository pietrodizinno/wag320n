include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1


export BIN = pppd
export OBJ = 
OBJ += main.o
OBJ += magic.o
OBJ += fsm.o
OBJ += lcp.o
OBJ += ipcp.o
OBJ += upap.o
OBJ += chap.o
OBJ += md5.o
OBJ += ccp.o
OBJ += auth.o
OBJ += options.o
OBJ += demand.o
OBJ += utils.o
OBJ += sys-linux.o
OBJ += ipxcp.o
OBJ += tty.o
OBJ += md4.o
OBJ += chap_ms.o
OBJ += tdb.o

export SLIB =
export DLIB = 

export CFLAGS = -Os -pipe -Wall -D_linux_=1 -DHAVE_PATHS_H -DIPX_CHANGE -DHAVE_MMAP -D_DISABLE_SERIAL_ 
CFLAGS += -DCHAPMS=1
CFLAGS += -DUSE_CRYPT=1
CFLAGS += -DHAVE_CRYPT_H=1
CFLAGS += -DPLUGIN

export LDFLAGS = -L./plugins/pppoe -L./plugins -L$(TK_APPS_PATH)/linux-atm-2.4.0/src/lib/.libs -L $(TK_APPS_PATH)/libs/libnv -L$(TK_APPS_PATH)/../target/lib
export ARFLAGS =
export STFLAGS =

ifeq ($(IPV6), 1)
	OBJ += ipv6cp.o eui64.o
	CFLAGS += -DINET6=1
endif

export INC = -I./ -I../include  -I$(TK_APPS_PATH)/libs/libnv
export KER_INC =
export LDLIBS = -lpppoe -lpppoatm -latm -lcrypt -l nv

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib
