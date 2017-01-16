ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/../../../..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/pppoe.mk


%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $(KER_INC) -o $@ $<

all: $(DLIB) $(BIN)

$(DLIB): $(OBJ1) $(OBJ2)
	$(LINK) $(CFLAGS) $(LDFLAGS1) $(LDLIBS1) -o $@ $^
	
$(BIN): $(OBJ1) $(OBJ3)
	$(LINK) $(CFLAGS) $(LDFLAGS2) $(LDLIBS2) -o $@ $^

install: $(DLIB) $(BIN)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) -t $(INSTALL_DIR) $(BIN)
	install -d $(INSTALL_LIB_DIR)
	$(INSTALL) -t $(INSTALL_LIB_DIR) $(DLIB)

clean:
	rm -f *~ *.o $(DLIB) $(BIN)

.PHONY: all install clean
