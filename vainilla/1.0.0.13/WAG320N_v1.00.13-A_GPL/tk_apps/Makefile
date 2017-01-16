export TK_APPS_PATH := $(shell pwd)

export TK_TARGET_PATH := $(TK_APPS_PATH)/../target

include $(TK_APPS_PATH)/tk_config/project_name.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps.mk

BUILDTIME=$(shell date +%FT%T)

all:
	@for i in ${TK_MODULES} ;  do                                                     \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;34m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i || exit 1 ;                                                   \
	done

install:	
	@for i in ${TK_MODULES} ;  do                                                     \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;35m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i install || exit 1 ;                                           \
	done
ifeq ($(HNAP), 1)
	rm -rf $(TK_TARGET_PATH)/etc/hnapinfo
	echo "Verdor: $(VERDOR_NAME)" > $(TK_TARGET_PATH)/etc/hnapinfo
	echo "Model Descript: $(MODULE_DESC)" >> $(TK_TARGET_PATH)/etc/hnapinfo
	echo "ModelName: $(MODULE_NAME)" >> $(TK_TARGET_PATH)/etc/hnapinfo
	echo "Firmware Version: $(FW_VER)" >> $(TK_TARGET_PATH)/etc/hnapinfo
	echo "Build Time: $(BUILDTIME)" >> $(TK_TARGET_PATH)/etc/hnapinfo
endif
##make System Info file
	echo "Verdor: $(MODULE)" > $(TK_TARGET_PATH)/etc/sysinfo
	echo "ModelName: $(MODULE_NAME)" >> $(TK_TARGET_PATH)/etc/sysinfo
	echo "Firmware Version: $(FW_VER) , $(BUILDTIME)" >> $(TK_TARGET_PATH)/etc/sysinfo
	echo "GUI Version: $(FW_VER)_$(UI_VER)" >> $(TK_TARGET_PATH)/etc/sysinfo
	echo "Boot Version: $(BOOT_VER)" >> $(TK_TARGET_PATH)/etc/sysinfo
	echo "Hardware Version: $(HW_VER)" >> $(TK_TARGET_PATH)/etc/sysinfo
	echo "Wireless Driver Version: $(WIFI_VER)" >> $(TK_TARGET_PATH)/etc/sysinfo
	
	#echo $(VERSION) > $(FS_DIR)/etc/version
	#echo $(SUB_VER) > $(FS_DIR)/etc/sub_version
	echo $(HW_ID) > $(FS_DIR)/etc/hw_id
    
clean:
	@echo "TK_MODULES=${TK_MODULES}"	
	@for i in ${TK_MODULES} ;  do                                                     \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;36m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i clean || exit 1 ;                                             \
	done

.PHONY: all install clean
