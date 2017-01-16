#-----------------------------------------------------
TOPDIR  :=  $(shell /bin/pwd)
TOPDIR  :=  $(TOPDIR)/../../../../

include $(TOPDIR)/cross.mak
#-----------------------------------------------------
export INSTALL := install
export TK_INSTALL_PATH := $(TOPDIR)/target
export INSTALL_DIR_LIB = $(TK_INSTALL_PATH)/lib

localedir=
CFLAGS +=-fPIC -I. -I../include -DLOCALEDIR=\"$(localedir)\" -DHAVE_CONFIG_H -DLIBDIR=\"\" -DUSE_DOS

OBJS=iconv.o
LIBCHARSET_OBJECTS = ../libcharset/lib/localcharset.o

all: $(OBJS) $(LIBCHARSET_OBJECTS)
	$(CC) $(CFLAGS) -shared -Wl,-soname,libiconv.so.0 -o libiconv.so.0.0 $(OBJS) $(LIBCHARSET_OBJECTS)
	$(STRIP) libiconv.so.0.0
	ln -sf libiconv.so.0.0 libiconv.so.0
	ln -sf libiconv.so.0 libiconv.so
#	cp -afv libiconv* $(INSTALL_DIR_LIB)

install:all
	cp -afv  libiconv.so*  $(INSTALL_DIR_LIB)
	
iconv.o: iconv.c
	$(CC) -c $(CFLAGS) iconv.c

clean:
	@echo Cleaning up
	rm -f *.[od]
	rm -f ../libcharset/lib/localcharset.[od]
	rm -f libiconv*

#-----------------------------------------------------
#include $(TOPDIR)/rules.mk
#-----------------------------------------------------
