#-----------------------------------------------------
TOPDIR  :=  $(shell /bin/pwd)
TOPDIR  :=  $(TOPDIR)/../..

include $(TOPDIR)/cross.mak
#-----------------------------------------------------
export INSTALL := install
export TK_INSTALL_PATH := $(TOPDIR)/target
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin

CFLAGS  = -Os -s -Wall  -D_FILE_OFFSET_BITS=64 

all: mkdosfs

install: mkdosfs
	install -d $(INSTALL_DIR)
	cp -af mkdosfs $(INSTALL_DIR)
	
mkdosfs: mkdosfs.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -rf *~ *.o *.so mkdosfs

