ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/push_button.mk


all: #mklnks
	$(MAKE) .all

ifeq ($(KVER), 2.4) ################## for 2.4 version kernel ########################

%.o: %.c
	$(CC) $(CFLAGS) $(INC) $(KER_INC) -DKBUILD_BASENAME=% -DKBUILD_MODNAME=% -c $<

.all: $(KER_MOD)

install: $(KER_MOD)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) -t $(INSTALL_DIR) $^
	
clean:
	rm -f $(KER_MOD) *~

else ifeq ($(KVER), 2.6) ############# for 2.6 version kernel ########################

PWD = $(shell pwd)

.all: 
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD)

install: 
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) *.ko
endif
	install -d $(INSTALL_DIR)
	cp -f *.ko $(INSTALL_DIR)

clean:
	rm -rf .*.cmd *.o *.mod.c *.ko .tmp_versions

endif ################################ end ###########################################

mklnks:
	./mklnks.sh $(TK_APPS_PATH)

.PHONY: all install clean .all

