ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/wsc_monitor.mk

all:  $(PROGRAM_NAME2)


install:
	install -m 755 $(PROGRAM_NAME2) $(INSTALL_DIR)
#	$(STRIP) $(INSTALL_DIR)/smd
#	$(STRIP) $(INSTALL_DIR)/$(PROGRAM_NAME)
	$(STRIP) $(INSTALL_DIR)/$(PROGRAM_NAME2)

$.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<  $(CXXFLAGS)

clean:
	#rm -f *.o $(PROGRAM_NAME) $(PROGRAM_NAME2) *.a
