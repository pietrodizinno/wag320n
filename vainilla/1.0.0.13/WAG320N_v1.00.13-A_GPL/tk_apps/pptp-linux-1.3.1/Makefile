# $Id: Makefile,v 1.1.1.1 2009-02-18 08:33:39 andy_huang Exp $
ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/pptp-linux.mk

VERSION=1.3.1
RELEASE=

#################################################################
# CHANGE THIS LINE to point to the location of your pppd binary.
PPPD = /usr/sbin/pppd
#################################################################

#BINDIR=$(DESTDIR)/usr/sbin
#MANDIR=$(DESTDIR)/usr/share/man/man8

#CC	= gcc
RM	= rm -f
OPTIMIZE= -Os -s -Wall
#DEBUG	= -g
INCLUDE = 
CFLAGS  = -Wall $(OPTIMIZE) $(DEBUG) $(INCLUDE)
LIBS	= -lutil
LDFLAGS	=

PPTP_BIN = pptp

PPTP_OBJS = pptp.o pptp_gre.o ppp_fcs.o \
            pptp_ctrl.o dirutil.o vector.o \
            inststr.o util.o version.o \
	    pptp_quirks.o orckit_quirks.o pqueue.o pptp_callmgr.o

PPTP_DEPS = pptp_callmgr.h pptp_gre.h ppp_fcs.h util.h \
	    pptp_quirks.h orckit_quirks.h config.h pqueue.h

all: config.h $(PPTP_BIN)

$(PPTP_BIN): $(PPTP_OBJS) $(PPTP_DEPS)
	$(CC) $(CFLAGS) -o $(PPTP_BIN) $(PPTP_OBJS) $(LDFLAGS) $(LIBS)

config.h: 
	echo "/* text added by Makefile target config.h */" > config.h
	echo "#define PPTP_LINUX_VERSION \"$(VERSION)$(RELEASE)\"" >> config.h
	echo "#define PPPD_BINARY \"$(PPPD)\"" >> config.h
clean:
	rm  -f *.o pptp config.h

install: $(PPTP_BIN)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) $(PPTP_BIN) $(INSTALL_DIR)

.PHONY: all install clean

