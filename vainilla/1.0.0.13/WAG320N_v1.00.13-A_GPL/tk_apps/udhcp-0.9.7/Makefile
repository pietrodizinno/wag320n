ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/udhcp.mk

# udhcp makefile

prefix=$(TI_FILESYSTEM)
SBINDIR=/sbin
USRSBINDIR=${prefix}/sbin
USRBINDIR=${prefix}/bin
USRSHAREDIR=${prefix}/share

# Uncomment this to get a shared binary. Call as udhcpd for the server,
# and udhcpc for the client
COMBINED_BINARY=1

# Uncomment this for extra output and to compile with debugging symbols
#DEBUG=0

# Uncomment this to output messages to syslog, otherwise, messages go to stdout
CFLAGS += -Os -s -Wall
CFLAGS +=  -I../libs/libnv -I../libs/libscfg -I../libs/libflash

INSTALL = install

VER := 0.9.7


OBJS_SHARED = options.o socket.o packet.o pidfile.o
DHCPD_OBJS = dhcpd.o arpping.o files.o leases.o serverpacket.o
DHCPC_OBJS = dhcpc.o clientpacket.o script.o

ifdef COMBINED_BINARY
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(DHCPC_OBJS) $(OBJS_SHARED) frontend.o
CFLAGS += -DCOMBINED_BINARY
else
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(OBJS_SHARED)

EXEC2 = udhcpc
OBJS2 = $(DHCPC_OBJS) $(OBJS_SHARED)
endif

EXEC3 = dumpleases
OBJS3 = dumpleases.o

BOOT_PROGRAMS = udhcpc
DAEMONS = udhcpd
COMMANDS = dumpleases

ifdef SYSLOG
CFLAGS += -DSYSLOG
endif

CFLAGS += -W -Wall -Wstrict-prototypes -DVERSION='"$(VER)"'

ifeq ($(DLNA_CERTIFICATE), 1)
CFLAGS += -DAUTO_IP
endif

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(LED), 1)
CFLAGS += -DLED
endif

CFLAGS +=-DRONSCODE
ifeq ($(WIFI), 1)
CFLAGS += -D_WIFI_BR0_
endif

all: $(EXEC1) $(EXEC2)

$(OBJS1) $(OBJS2) $(OBJS3): *.h Makefile
$(EXEC1) $(EXEC2) $(EXEC3): Makefile

.c.o:
	$(CC) -c $(CFLAGS) $<

$(EXEC1): $(OBJS1)
	$(CC) $(OBJS1) -o $(EXEC1) -L../libs/libscfg -lscfg -L../libs/libnv -lnv
	$(STRIP) $@

$(EXEC2): $(OBJS2)
	$(CC) $(OBJS2) -o $(EXEC2)
	$(STRIP) $@

$(EXEC3): $(OBJS3)
	$(CC) $(OBJS3) -o $(EXEC3)
	$(STRIP) $@

install:
	rm -f udhcpc
	ln -s udhcpd udhcpc
	cp -a udhcpd $(TK_INSTALL_PATH)/usr/sbin
	cp -a udhcpc $(TK_INSTALL_PATH)/usr/sbin
clean:
	rm -f udhcpd udhcpc dumpleases *.o core


