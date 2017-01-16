#-----------------------------------------------------
TOPDIR  :=  $(shell /bin/pwd)
TOPDIR  :=  $(TOPDIR)/../../

include $(TOPDIR)/cross.mak
#-----------------------------------------------------
export INSTALL := install
export TK_INSTALL_PATH := $(TOPDIR)/target
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin

SERVICES =    \"/etc/services\"
INETD_CONF =  \"/etc/inetd.conf\"

EXEC = inetd
OBJS = inetd.o config.o

CFLAGS += -DSERVICES=$(SERVICES) -DINETD_CONF=$(INETD_CONF)
CFLAGS += -Wall

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS$(LDLIBS_$@))
	$(STRIP) inetd

clean:
	rm -f $(EXEC) *.elf *.gdb *.[do]

install:
	cp -pdR $(EXEC) $(INSTALL_DIR)

