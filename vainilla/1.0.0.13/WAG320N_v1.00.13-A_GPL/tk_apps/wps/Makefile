ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/wps.mk

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(PROGRAM_NAME)
	
#$(PROGRAM_NAME): wps_gpio_dslcpe.o wps_monitor.lib libwps.so
#	$(CC) -o $@ $^ $(LDFLAGS)

wps_monitor.lib:

libwps.so:

install:
	install -m 755 $(PROGRAM_NAME) $(INSTALL_DIR)
	$(STRIP) $(INSTALL_DIR)/$(PROGRAM_NAME)
	install -m 755 wps_ap $(INSTALL_DIR)/../../bin
	$(STRIP) $(INSTALL_DIR)/../../bin/wps_ap
	install -m 755 wps_enr $(INSTALL_DIR)/../../bin
	$(STRIP) $(INSTALL_DIR)/../../bin/wps_enr
	install -m 755 libwps.so $(INSTALL_LIB_DIR)
	$(STRIP) $(INSTALL_LIB_DIR)/libwps.so

clean:
	#rm -f *.o $(PROGRAM_NAME) *.a
