#! /usr/bin/make

EXTRAS+=libiptc/libiptc.a

DEVEL_LIBS+=libiptc/libiptc.a

ifndef TOPLEVEL_INCLUDED
local:
	cd .. && $(MAKE) $(KERN_TARGETS) $(SHARED_LIBS) $(EXTRAS)

else
EXTRA_DEPENDS+=libiptc/libip4tc.d

ifeq ($(DO_IPV6), 1)
EXTRA_DEPENDS+= libiptc/libip6tc.d
libiptc/libiptc.a: libiptc/libip4tc.o libiptc/libip6tc.o
	$(AR) rv libiptc/libiptc.a libiptc/libip4tc.o libiptc/libip6tc.o
else
libiptc/libiptc.a: libiptc/libip4tc.o
	$(AR) rv libiptc/libiptc.a libiptc/libip4tc.o
endif

libiptc/libip4tc.d libiptc/libip6tc.d: %.d: %.c
	@-$(CC) -M -MG $(CFLAGS) $< | sed -e 's@^.*\.o:@$*.d libiptc/libiptc.a($*.o):@' > $@
endif
