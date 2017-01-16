ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/../../..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/pppoatm.mk


%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $(KER_INC) -o $@ $<

all: $(DLIB)

$(DLIB): $(OBJ)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

install: $(DLIB)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_LIB_DIR)
	$(INSTALL) -t $(INSTALL_LIB_DIR) $(DLIB)

clean:
	rm -f *~ *.o $(DLIB)

.PHONY: all install clean

