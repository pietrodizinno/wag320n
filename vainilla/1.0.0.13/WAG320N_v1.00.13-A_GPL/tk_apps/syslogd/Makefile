ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/syslogd.mk


%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $(KER_INC) -o $@ $<

all: $(BIN1) $(BIN2)

$(BIN1): $(OBJ1)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^
	
$(BIN2): $(BIN1)
	ln -sf $< $@

install: $(BIN1) $(BIN2)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $(BIN1)
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) $(BIN1) $(INSTALL_DIR)
	cp -a $(BIN2) $(INSTALL_DIR)
	
clean:
	rm -f $(BIN1) $(BIN2) *.o

.PHONY: all install clean
