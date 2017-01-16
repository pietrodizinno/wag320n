ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/dnrd.mk

build: $(BIN)

clean:
	rm -f $(BIN) core tags ID *.o *.d *~ *.bak *.gdb

install: $(BIN)
	cp -f $(BIN) $(TK_INSTALL_PATH)/usr/sbin/

sources = args.c check.c dns.c lib.c master.c query.c relay.c srvnode.c udp.c \
	  cache.c common.c domnode.c main.c qid.c rand.c sig.c tcp.c


$(BIN):	$(sources:.c=.o)
	$(CC) $(CFLAGS)  $^ -o $@ $(LIBS)
	
%.o: %.c
	$(CC) $(CFLAGS)  -o $@ -c $^ 

%.d: %.c
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		      [ -s $@ ] || rm -f $@'
