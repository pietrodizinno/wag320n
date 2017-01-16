ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

PROGS     = utelnetd

INSTDIR   = ../../target/usr/sbin/
INSTMODE  = 0755
INSTOWNER = root
INSTGROUP = root


OBJS = utelnetd.o

all: $(PROGS)

CFLAGS   += -DSHELLPATH=\"$(run_prefix)/bin/login\"

ifneq ("","$(DEBUG)")
CFLAGS   += -DDEBUG
endif

$(PROGS): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -Os -s -o $@

.PHONY: install
install: $(PROGS)
	cp utelnetd $(INSTDIR)

.PHONY: clean
clean:
	rm -f $(PROGS) *.o core
