include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = tc/tc

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin





