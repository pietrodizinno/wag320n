ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/mini_httpd.mk


%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $(KER_INC) -o $@ $<

%.h: %.txt
	rm -f $@
	sed < $< > $@ \
	  -e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
	  -e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'
#for trennmicro test, must be synchronous with src/UI/cgi/setup/Makefile
HSET_HTM=1


ifeq ($(HSET_HTM),1)
    CFLAGS += -DHSET_HTM
endif

ifeq ($(TMICRO), 1)
CFLAGS+= -D_TMICRO_
endif 

all: $(HDR) $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

install: $(BIN)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_DIR)
#	$(INSTALL) -t $(INSTALL_DIR) $^ $(PEM)
	$(INSTALL) $(PEM) $(INSTALL_DIR)
	$(INSTALL) $(BIN) $(INSTALL_DIR)

clean:
	rm -f $(BIN) $(HDR) *.o

.PHONY: all install clean


