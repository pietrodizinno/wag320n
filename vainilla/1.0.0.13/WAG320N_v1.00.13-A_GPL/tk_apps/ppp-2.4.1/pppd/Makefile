ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/../..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/pppd.mk


%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $(KER_INC) -o $@ $<

all: $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

install: $(BIN)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) -t $(INSTALL_DIR) $^

clean:
	rm -f *~ *.o $(BIN)

.PHONY: all install clean


