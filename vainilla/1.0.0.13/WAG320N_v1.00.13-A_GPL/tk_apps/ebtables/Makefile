ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/ebtable.mk

all: $(PROGRAM_NAME)

communication.o: communication.c include/ebtables_u.h
	$(CC) $(CFLAGS) $(PROGSPECS) -c -o $@ $< -I$(KERNEL_INCLUDES)

getethertype.o: getethertype.c include/ethernetdb.h
	$(CC) $(CFLAGS) $(PROGSPECS) -c -o $@ $< -Iinclude/

ebtables.o: ebtables.c include/ebtables_u.h
	$(CC) $(CFLAGS) $(PROGSPECS) -c -o $@ $< -I$(KERNEL_INCLUDES)
  
ether_addr.o: ether_addr.c
	$(CC) $(CFLAGS) $(PROGSPECS) -c -o $@ $< -I$(KERNEL_INCLUDES)

$(PROGRAM_NAME): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -I$(KERNEL_INCLUDES)

.PHONY: install
install:
	install -m 755 $(PROGRAM_NAME) $(INSTALL_DIR)/
ifeq ($(STRIP_ENABLE), 1)
	$(STRIP) $(INSTALL_DIR)/$(PROGRAM_NAME)
endif

.PHONY: clean
clean:
	rm -f $(PROGRAM_NAME)
	rm -f *.o *.c~
	rm -f extensions/*.o extensions/*.c~
