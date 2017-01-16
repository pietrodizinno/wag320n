ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/iproute2.mk

# Path to parent kernel include files directory
KERNEL_INCLUDE=../../kernel_2.6/linux/include

# Path to db_185.h include
DBM_INCLUDE:=/usr/include

DEFINES= -DRESOLVE_HOSTNAMES

#options if you have a bind>=4.9.4 libresolv (or, maybe, glibc)
LDLIBS=-lresolv
ADDLIB=

#options for decnet
ADDLIB+=dnet_ntop.o dnet_pton.o

#options for ipx
ADDLIB+=ipx_ntop.o ipx_pton.o

CCOPTS = -D_GNU_SOURCE -O2 -Wstrict-prototypes -Wall -g
CFLAGS = $(CCOPTS) -I../include $(DEFINES)

LDLIBS += -L../lib -lnetlink -lutil

SUBDIRS=lib tc

LIBNETLINK=../lib/libnetlink.a ../lib/libutil.a

all: Config
	@for i in $(SUBDIRS); \
	do $(MAKE) $(MFLAGS) -C $$i; done

Config:
	./configure $(KERNEL_INCLUDE)

install:
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $(BIN)
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) $(BIN) $(INSTALL_DIR)


clean:
	@for i in $(SUBDIRS) doc; \
	do $(MAKE) $(MFLAGS) -C $$i clean; done

clobber: clean
	rm -f Config


.EXPORT_ALL_VARIABLES:

