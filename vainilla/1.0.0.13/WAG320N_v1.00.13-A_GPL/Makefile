include Rules.mk
# Environment Variables
TARGET_HOME = $(shell pwd)
APPS_DIR  = $(TARGET_HOME)/tk_apps
DRIVER_DIR   = $(TARGET_HOME)/driver
KERNEL_DIR = $(TARGET_HOME)/kernel
FS_DIR  = $(TARGET_HOME)/target
FIRMWARE_DIR =$(TARGET_HOME)/tools/makeimage
TOOLS_DIR  = $(TARGET_HOME)/tools
IMAGE_DIR  = $(TARGET_HOME)/image
UI_DIR = $(TARGET_HOME)/UI
STRIP = mips-linux-uclibc-strip

TOOL_CHAIN_DIR = $(TARGET_HOME)/uclibc-crosstools-3.4.2/uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin

export TARGET_HOME APPS_DIR FS_DIR

export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:$(TOOL_CHAIN_DIR)

export BSP_DIR = $(TARGET_HOME)/$(BCMBSP_DIR)

# add some path variable used by broadcom apps, wlctl eg.
#export BSP_VER
export BRCM_CHIP=6358
export BRCM_BOARD=bcm963xx
export BRCMDRIVERS_DIR=$(BSP_DIR)/bcmdrivers
export BSP_SHARED_DIR=$(BSP_DIR)/shared
export INC_BRCMDRIVER_PUB_PATH=$(BRCMDRIVERS_DIR)/opensource/include
export INC_BRCMDRIVER_PRIV_PATH=$(BRCMDRIVERS_DIR)/broadcom/include
export INC_ENDPOINT_PATH=$(BRCMDRIVERS_DIR)/broadcom/char/endpoint/bcm9$(BRCM_CHIP)/inc
export INC_ADSLDRV_PATH=$(BRCMDRIVERS_DIR)/broadcom/char/adsl/impl1
export INC_BRCMSHARED_PUB_PATH=$(BSP_SHARED_DIR)/opensource/include
export INC_BRCMSHARED_PRIV_PATH=$(BSP_SHARED_DIR)/broadcom/include
export INC_BRCMBOARDPARMS_PATH=$(BSP_SHARED_DIR)/opensource/boardparms
export INC_FLASH_PATH=$(BSP_SHARED_DIR)/opensource/flash
export BSP_APPS_PRIV_DIR = $(BSP_DIR)/userspace/private/apps
export BSP_PROFILE_DIR=$(BSP_DIR)/targets/$(MODULE_NAME)/fs.install
#export BRCM_EXTRA_CFLAGS= -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -DCONFIG_SERCOMM_CODE=y -DBSP_VER=$(BSP_VER) 

LOADER = $(IMAGE_DIR)/cfe6358.bin

ifeq ($(ANNEX),A)
KERNEL=$(KERNEL_DIR)/vmlinux.lz.a
else
KERNEL=$(KERNEL_DIR)/vmlinux.lz.b
endif

ifeq ($(ANNEX),B)
	FS_NAME=$(IMAGE_DIR)/fsb.bin
else
	FS_NAME=$(IMAGE_DIR)/fsa.bin
endif 

ifeq ("$(SUB_VER)","00")
FW_SUB_VERSION ?=
else
FW_SUB_VERSION ?= \($(SUB_VER)\)
endif

.PHONY: all auto premake clean clean_all

all: build_target make_links apps apps_install fs_and_bin 
auto: build_kernel all
premake: 
	./prepare.sh

.PHONY:build_kernel
build_kernel:
ifeq ($(ANNEX), A)
	cd $(BCMBSP_DIR) && ./build_ANNEX_A.sh && cd -
else
	cd $(BCMBSP_DIR) && ./build_ANNEX_B.sh && cd -
endif

.PHONY:build_target
build_target:
	rm -rf $(FS_DIR)
	tar zxvf $(TARGET_HOME)/target.tgz 
	ln -f -s /tmp/var/log/log $(FS_DIR)/dev/log
	mkdir -p $(FS_DIR)/tmp/var/log
	echo -n > $(FS_DIR)/tmp/var/log/log
	chmod 666 $(FS_DIR)/tmp/var/log/log
	
.PHONY:make_links
make_links: 
	./prepare.sh

#User application build rule
.PHONY: apps
apps:
	@echo "----------------------------------------"
	@echo "Compilation of client applications..."
	$(MAKE) -C $(APPS_DIR)
	@echo "----------------------------------------"

.PHONY: apps_install
apps_install:
	@echo "----------------------------------------"
	@echo "Install apps"
	cd $(APPS_DIR) && make install
	@echo "----------------------------------------"
	find $(FS_DIR)/ -name CVS | xargs rm -rf
	find $(FS_DIR)/ -name Makefile | xargs rm -rf
	find $(FS_DIR)/ -name _desktop.ini | xargs rm -rf

.PHONY:fs_and_bin
fs_and_bin:
	find $(FS_DIR)/lib/modules/2.6.21.5/extra -name '*.ko' | xargs $(STRIP) --strip-unneeded
	rm -f $(FS_NAME)
	$(BSP_DIR)/hostTools/mksquashfs $(FS_DIR) $(FS_NAME) -be -noappend -all-root -D ./tools/device_table.txt
	$(BSP_DIR)/hostTools/bcmImageBuilder --output $(IMAGE_DIR)/bcm963xx_fs_kernel --chip 6358 --board "96358GWVG" --blocksize 64 --cfefile $(LOADER) --rootfsfile $(FS_NAME) --kernelfile  $(KERNEL)
ifeq ("$(ANNEX)","A")
	cd $(IMAGE_DIR) && cp -f wag320n.pid.a wag320n.pid && ./bb && mv wag320n.bin wag320n_$(ANNEX)_$(FW_VER)$(FW_SUB_VERSION).bin
endif
ifeq ("$(ANNEX)","B")
	cd $(IMAGE_DIR) && cp -f wag320n.pid.b wag320n.pid && ./bb && mv wag320n.bin wag320n_$(ANNEX)_$(FW_VER)$(FW_SUB_VERSION).bin
endif

.PHONY:make_new_firmware
make_new_firmware:
	$(MAKE) -C $(IMAGE_DIR)

clean:
	rm -rf $(TARGET_HOME)/target
	$(MAKE) -C $(APPS_DIR) clean

clean_all:
	$(MAKE) -C $(FIRMWARE_DIR) clean
	$(MAKE) -C $(APPS_DIR) clean
	$(MAKE) -C $(TOOLS_DIR) clean
	rm -rf $(TARGET_HOME)/target

tool_chain:
	$(MAKE) -C $(TOOLS_DIR) mipsel-linux-uclibc

%-build:
	$(MAKE) -C tk_apps/$(*-build) make

%-clean:
	$(MAKE) -C tk_apps/$(*-clean) clean
	
