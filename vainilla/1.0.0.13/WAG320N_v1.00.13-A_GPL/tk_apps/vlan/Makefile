ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/vconfig.mk

KERNEL_INCLUDES2=$(BRCMDRIVERS_DIR)/opensource/include/bcm963xx

CFLAGS= -Wall -Wl,-allow-shlib-undefined -I$(KERNEL_INCLUDES2) -s -Os -fomit-frame-pointer
ifeq ($(strip $(BUILD_VCONFIG)), static)
CFLAGS += -DBUILD_STATIC
endif
all: $(BIN)

ifdef NetBSD
FW_FLAGS="-DUSE_IPF"
endif

#install:
#	install -m 755 vconfig $(INSTALL_DIR)/bin
#	$(STRIP) $(INSTALL_DIR)/bin/vconfig

install: $(BIN)
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(STFLAGS) $^
endif
	install -d $(INSTALL_DIR)
	$(INSTALL) $(BIN) $(INSTALL_DIR)

dynamic: $(BIN) install

static: vconfig.a

$(BIN): vconfig.c
	$(CC) $(CFLAGS) -o vconfig vconfig.c $(FW_FLAGS)

vconfig.a: vconfig.c
	$(CC) $(CFLAGS) -c -o vconfig.o vconfig.c $(FW_FLAGS)
	$(AR) rcs vconfig.a  vconfig.o

clean:
	-rm -f $(BIN) vconfig.o vconfig.a

.PHONY: all install clean
