CFLAGS = -Wall -O2 -s -D_GNU_SOURCE -DDEBUG -I ../include -I ./include -I ../../iptables-1.3.5/include/ -L ../../iptables-1.3.5/libiptc/ -L ../

#CFLAGS = -Wall -Os -D_GNU_SOURCE
RM = rm -f
INSTALL = install

BASEOBJS = igd_descgen.o igd_eventxml.o igd_globalvars.o igd_soap.o \
           igd_upnp.o port.o igd_permissions.o igd_redirect.o options.o
		   
LNXOBJS = linux/getifstats.o linux/iptcrdr.o

LIBS = -liptc -lminiupnpd

EXECUTABLES = igd_upnpd

.PHONY:	all clean install depend

all: $(BASEOBJS) $(LNXOBJS)
	$(CC) $(CFLAGS) $(BASEOBJS) $(LNXOBJS) $(LIBS) -o $(EXECUTABLES)

clean:
	$(RM) $(BASEOBJS) $(LNXOBJS) $(EXECUTABLES)

install:	
	cp $(EXECUTABLES) $(INSTALL_DIR)

