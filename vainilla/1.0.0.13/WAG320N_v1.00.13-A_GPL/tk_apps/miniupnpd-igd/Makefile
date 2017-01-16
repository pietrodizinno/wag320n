ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/miniupnpd-igd.mk

%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $(KER_INC) -o $@ $<

all: $(DLIB) $(SLIB) $(BIN)

$(DLIB): $(OBJ1)
	$(LINK) $(CFLAGS) $(LDFLAGS1) $(LDLIBS1) -o $@ $^
	
$(SLIB): $(OBJ1)
	$(AR) $(ARFLAGS) $@ $^

$(BIN): $(OBJ2)
	$(LINK) $(CFLAGS) $(LDFLAGS2) -o $@ $^ $(LDLIBS2)

install: $(BIN) $(DLIB)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) $(BIN) $(INSTALL_DIR)
	install -d $(INSTALL_LIB_DIR)
	$(INSTALL) $(DLIB) $(INSTALL_LIB_DIR)

clean:
	rm -f $(BIN) $(SLIB) $(DLIB) $(OBJ1) $(OBJ2)

.PHONY: all install clean



