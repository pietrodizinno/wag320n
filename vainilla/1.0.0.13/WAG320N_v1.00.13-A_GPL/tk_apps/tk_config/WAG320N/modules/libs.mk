include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk

export LIBS = 

ifeq ($(USB_STORAGE), 1)
	LIBS += libiconv-1.8
endif
LIBS += expat-2.0.1
