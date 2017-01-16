ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/linux-atm.mk


all:
	@for i in ${SUBDIRS} ;  do                                                        \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;34m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i all-am || exit 1 ;                                            \
	done

install:
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(BIN) $(DLIB)
endif
	install -d $(INSTALL_DIR)
#	install -t $(INSTALL_DIR) $(BIN)
	cp src/arpd/.libs/* $(INSTALL_DIR)
	install -d $(INSTALL_LIB_DIR)
	cp -af $(DLIB) $(INSTALL_LIB_DIR)

clean:
	@for i in ${SUBDIRS} ;  do                                                        \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;36m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i clean || exit 1 ;                                             \
	done

.PHONY: all install clean

