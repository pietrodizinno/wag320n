ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/wlctl.mk

.c.o:
	$(CC) -DPROTOTYPES $(CFLAGS) $(CFLAGS_EXTRA) -c $<

all: #lib$(PROGRAM_NAME) $(PROGRAM_NAME)

install::
	install -m 755 $(PROGRAM_NAME) $(INSTALL_DIR)/
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(INSTALL_DIR)/$(PROGRAM_NAME)
endif
	if [ -e lib$(PROGRAM_NAME).so ]; then cp -av lib$(PROGRAM_NAME).so $(INSTALL_LIB_DIR); fi

$(MAINOBJ): $(MAIN).c
	$(CC) $(CFLAGS) -o $@ -c $<

ifeq ($(strip $(BUILD_WLCTL_SHLIB)),1) 

lib$(PROGRAM_NAME): $(OBJS) 
	$(LD) -shared -soname lib$(PROGRAM_NAME).so -o lib$(PROGRAM_NAME).so $(OBJS) -L$(LIBDIR) -lc

$(PROGRAM_NAME): lib$(PROGRAM_NAME) $(MAINOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -L. -o $(PROGRAM_NAME) $(MAINOBJ) -l$(PROGRAM_NAME)

else

$(PROGRAM_NAME): $(MAINOBJ) $(OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM_NAME) $(MAINOBJ) $(OBJS)

lib$(PROGRAM_NAME):

endif

clean:
#	-rm -f $(PROGRAM_NAME) *.o  $(PROGRAM_NAME).a lib$(PROGRAM_NAME).so $(MAINOBJ)




