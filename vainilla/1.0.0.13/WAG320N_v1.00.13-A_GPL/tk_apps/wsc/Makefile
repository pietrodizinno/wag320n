ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/wsc.mk

all:

install:
	install -m 755 $(PROGRAM_NAME) $(INSTALL_DIR)
	$(STRIP) $(INSTALL_DIR)/$(PROGRAM_NAME)
	install -m 444 wsc_config_1a_ap.txt  $(INSTALL_DIR)/../../etc

$.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
#	rm -f *.o $(PROGRAM_NAME) *.a
