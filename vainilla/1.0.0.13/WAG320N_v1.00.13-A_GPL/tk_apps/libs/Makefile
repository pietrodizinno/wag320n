ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/libs.mk


all:
	@for i in ${LIBS} ;  do                                                           \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;34m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i || exit 1 ;                                                   \
	done

install:
	@for i in ${LIBS} ;  do                                                           \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;35m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i install || exit 1 ;                                           \
	done
    
clean:
	@for i in ${LIBS} ;  do                                                           \
            echo "-----------------------------------------------------------------"; \
            echo -e "\033[;36m              $(shell pwd)/$$i               \033[;0m"; \
            echo "-----------------------------------------------------------------"; \
            make -C $$i clean || exit 1 ;                                             \
	done

.PHONY: all install clean

