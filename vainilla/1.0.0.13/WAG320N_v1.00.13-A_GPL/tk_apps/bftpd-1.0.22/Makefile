# Generated automatically from Makefile.in by configure.
#-----------------------------------------------------
TOPDIR  :=  $(shell /bin/pwd)
TOPDIR  :=  $(TOPDIR)/../..

include $(TOPDIR)/cross.mak
#-----------------------------------------------------
export INSTALL := install
export TK_INSTALL_PATH := $(TOPDIR)/target
export INSTALL_DIR_USBIN = $(TK_INSTALL_PATH)/usr/sbin

VERSION=1.0.22
INSTALL=/usr/bin/install -c
CFLAGS+=-g -O2 -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -Wall -I. -I../libs/libstorage -I../libs/libiconv-1.8/include -I../libs/libfilename -DVERSION=\"$(VERSION)\" #-DDEBUG
CFLAGS+=-D__USE_UNICODE__
LIBS= -lcrypt -liconv -lstorage -lfilename -lscfg -lnv
prefix=/usr
mandir=${prefix}/man
sbindir=${exec_prefix}/sbin
DESTDIR=
DIRPAX=
PAX=$(DIRPAX)/append.o $(DIRPAX)/buffer.o $(DIRPAX)/cpio.o $(DIRPAX)/create.o $(DIRPAX)/extract.o $(DIRPAX)/fileio.o $(DIRPAX)/link.o $(DIRPAX)/list.o $(DIRPAX)/mem.o $(DIRPAX)/namelist.o $(DIRPAX)/names.o $(DIRPAX)/pass.o $(DIRPAX)/pathname.o $(DIRPAX)/pax.o $(DIRPAX)/port.o $(DIRPAX)/regexp.o $(DIRPAX)/replace.o $(DIRPAX)/tar.o $(DIRPAX)/ttyio.o $(DIRPAX)/warn.o $(DIRPAX)/wildmat.o

HEADERS=commands.h  dirlist.h login.h logging.h main.h mystring.h options.h targzip.h mypaths.h parsedir.h 
OBJS=commands.o  dirlist.o login.o logging.o main.o mystring.o options.o parsedir.o list.o
SRCS=commands.c  dirlist.c login.c logging.c main.c mystring.c options.c parsedir.c list.c

OBJS2LINK=$(OBJS)
ifneq ($(DIRPAX),)
  OBJS2LINK+=$(PAX)
endif
LDFLAGS:= -L../libs/libfilename 
LDFLAGS+= -L../libs/libiconv-1.8/lib
LDFLAGS+= -L../libs/libscfg
LDFLAGS+= -L../libs/libstorage
LDFLAGS+= -L../libs/libnv

all: bftpd

bftpd: $(OBJS)
	$(CC) $(OBJS2LINK) $(LDFLAGS) $(LIBS) -o $@ 
	$(STRIP) $@
	
$(OBJS): $(HEADERS) Makefile

install: 
	cp -pdR bftpd $(INSTALL_DIR_USBIN)
   
clean:
	rm -f *~ $(OBJS) bftpd mksources.finished *.bak 
	chmod 644 *.c *.h Makefile

newversion: clean
	cat Makefile.in | sed -e s/$(VERSION)/$(NEWVERSION)/g > Makefile.foo
	mv Makefile.foo Makefile.in
	./configure --enable-pax=pax --enable-libz --enable-pam

uninstall:
	rm -f $(DESTDIR)/$(prefix)/sbin/bftpd $(DESTDIR)/$(mandir)/man8/bftpd.8 \
		$(DESTDIR)/etc/bftpd.conf

distribute: install
	rm -rf dist
	mkdir dist
	# Build source tarball
	rm -rf bftpd-$(VERSION)
	mkdir bftpd-$(VERSION)
	autoconf
	autoheader configure.in > config.h.in
	mkdir bftpd-$(VERSION)/doc
	cp CHANGELOG COPYING Makefile.in $(SRCS) $(HEADERS) \
	configure.in configure config.h.in bftpd.conf install-sh mksources \
	bftpd.spec.in acconfig.h bftpd.8 ftpmotd bftpd-$(VERSION)
	cp -r doc bftpd-$(VERSION)
	cd bftpd-$(VERSION)/doc/en && sgml2txt bftpddoc-en.sgml
	cd bftpd-$(VERSION) && ln -s doc/en/bftpddoc-en.txt INSTALL
	cd bftpd-$(VERSION) && ln -s doc/en/bftpddoc-en.txt README
	rm -f -r debian/tmp
	cp -a debian bftpd-$(VERSION)
	tar c bftpd-$(VERSION) | gzip -v9 > dist/bftpd-$(VERSION).tar.gz
	#cp -lr pax bftpd-$(VERSION)
	#tar c bftpd-$(VERSION) | gzip -v9 > bftpd-$(VERSION)-pax.tar.gz
	# Build binary RPM
	./configure && make install
	cat bftpd.spec.in | sed -e 's/VERSION/$(VERSION)/g' \
	> bftpd-$(VERSION)-1.spec
	rpm -bb bftpd-$(VERSION)-1.spec
	rm -f bftpd-$(VERSION)-1.spec
	cp -af /usr/src/redhat/RPMS/i386/bftpd-$(VERSION)-1.i386.rpm \
	dist/bftpd-$(VERSION).i386.rpm
	# Build binary DEB
	#mv bftpd-$(VERSION)-pax.tar.gz bftpd-$(VERSION).tar.gz
	#cd bftpd-$(VERSION) && (\
	#	cat debian/changelog.in | sed -e 's/VERSION/$(VERSION)/g' \
	#		> debian/changelog; \
	#	dpkg-buildpackage; \
	#) && cd ..
	rm -f dist/bftpd-$(VERSION).tar.gz
	#cp bftpd_$(VERSION)-1_i386.deb dist
	rm -f bftpd_*
	# Remove temporary directory
	rm -rf bftpd-$(VERSION)
