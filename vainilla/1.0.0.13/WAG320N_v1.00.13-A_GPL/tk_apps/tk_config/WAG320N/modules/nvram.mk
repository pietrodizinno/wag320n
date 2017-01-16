include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE := 1

export INSTALL := cp -f
export INSTALL_LIB_DIR := $(TK_INSTALL_PATH)/lib 
export INSTALL_BIN_DIR := $(TK_INSTALL_PATH)/usr/sbin

export LINK := $(CC)
export CFLAGS := -Os -Wall -I$(TK_KERNEL_PATH)/src/include
export CFLAGS2 := $(CFLAGS) -fPIC
export LDFLAGS := -Os -shared -Wl,-soname,libnvram.so

