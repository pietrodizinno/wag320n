include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = igd/igd_upnpd

export OBJ1 =   
OBJ1 += miniupnpd.o
OBJ1 += upnphttp.o
OBJ1 += upnpdescgen.o
OBJ1 += upnpsoap.o
OBJ1 += upnpreplyparse.o
OBJ1 += minixml.o
OBJ1 += getifaddr.o
OBJ1 += daemonize.o
OBJ1 += minissdp.o
OBJ1 += upnpevent.o

export OBJ2 =             
OBJ2 += igd/igd_descgen.o
OBJ2 += igd/igd_eventxml.o
OBJ2 += igd/igd_globalvars.o
OBJ2 += igd/igd_soap.o
OBJ2 += igd/igd_upnp.o
OBJ2 += igd/port.o
OBJ2 += igd/igd_permissions.o
OBJ2 += igd/igd_redirect.o
OBJ2 += igd/options.o
OBJ2 += igd/linux/getifstats.o
OBJ2 += igd/linux/iptcrdr.o

export SLIB = libminiupnpdstatic.a
export DLIB = libminiupnpd.so

export CFLAGS = -Wall -O2 -D_GNU_SOURCE
export LDFLAGS1 = -shared -Wl,-soname,libminiupnpd.so
export LDFLAGS2 = -L./ -L$(TK_APPS_PATH)/iptables-1.3.5/libiptc -L$(TK_APPS_PATH)/libs/libnv -L$(TK_APPS_PATH)/../target/lib
export ARFLAGS = -r
export STFLAGS =

export INC = -I./include -I./igd/include -I$(TK_APPS_PATH)/iptables-1.3.5/include  -I$(TK_APPS_PATH)/libs/libnv -I$(TK_APPS_PATH)/../kernel/bcmdriver/opensource/include/bcm963xx
export KER_INC = 
export LDLIBS1 = 
export LDLIBS2 = -lminiupnpd -liptc -lnv

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib

