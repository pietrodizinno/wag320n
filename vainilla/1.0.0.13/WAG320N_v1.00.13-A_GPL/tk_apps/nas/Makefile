ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/nas.mk

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all:
	#cp nas_save $(PROGRAM_NAME)
	
install:
	install -m 755 $(PROGRAM_NAME) $(INSTALL_DIR)
	install -m 755 eapd $(INSTALL_DIR)
	install -m 755 bcmupnp $(INSTALL_DIR)
	#install -m 755 lld2d $(INSTALL_DIR)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(INSTALL_DIR)/$(PROGRAM_NAME)
	$(STRIP) $(INSTALL_DIR)/eapd
	$(STRIP) $(INSTALL_DIR)/bcmupnp
	#$(STRIP) $(INSTALL_DIR)/lld2d
endif
	ln -sf ../usr/sbin/$(PROGRAM_NAME) $(INSTALL_DIR)/../../bin/nas4not

clean:
	#rm -f nas

