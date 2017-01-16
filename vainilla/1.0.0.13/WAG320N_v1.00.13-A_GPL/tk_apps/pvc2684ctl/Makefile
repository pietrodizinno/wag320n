#-----------------------------------------------------
TOPDIR  :=  $(shell /bin/pwd)
TOPDIR  :=  $(TOPDIR)/../..

include $(TOPDIR)/cross.mak
#-----------------------------------------------------
export INSTALL := install
export TK_INSTALL_PATH := $(TOPDIR)/target
export INSTALL_DIR_USBIN = $(TK_INSTALL_PATH)/usr/sbin
EXECS = pvc2684ctl pvc2684d

ifeq ($(strip $(BUILD_PVC2684CTL)),static)
CFLAGS += -DBUILD_STATIC
endif

INCLUDE = $(TOPDIR)/kernel/src/include


CFLAGS += -Wall -I$(INCLUDE)
LIBS+= -L$(TOPDIR)/tk_apps/linux-atm-2.4.0/src/lib/.libs -latm

all: $(EXECS) 

dynamic: libatm $(EXECS)

static: libatm pvc2684ctl.a pvc2684d.a
	
pvc2684d : pvc2684d.c brpvc.h
	$(CC) $(CFLAGS) -o $@ $< ${LIBS}

pvc2684ctl : pvc2684ctl.c brpvc.h
	$(CC) $(CFLAGS) -o $@ $< ${LIBS}

pvc2684ctl.a: pvc2684ctl.o
	$(AR) rcs pvc2684ctl.a  pvc2684ctl.o

pvc2684ctl.o: pvc2684ctl.c
	$(CC) $(CFLAGS) $(INCLUDE) -Os -c -o pvc2684ctl.o pvc2684ctl.c

pvc2684d.a: pvc2684d.o
	$(AR) rcs pvc2684d.a  pvc2684d.o

pvc2684d.o: pvc2684d.c
	$(CC) $(CFLAGS) $(INCLUDE) -Os -c -o pvc2684d.o pvc2684d.c

install: $(EXECS)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	cp -af pvc2684ctl $(INSTALL_DIR_USBIN)
	cp -af pvc2684d $(INSTALL_DIR_USBIN)

clean :
	rm -f *.o *~ pvc2684ctl pvc2684d core
