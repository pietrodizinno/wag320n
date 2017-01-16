ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/../..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/iproute2.mk

IPOBJ=ip.o ipaddress.o iproute.o iprule.o \
    rtm_map.o iptunnel.o ipneigh.o iplink.o \
    ipmaddr.o ipmonitor.o ipmroute.o \
    ipxfrm.o xfrm_state.o xfrm_policy.o

RTMONOBJ=rtmon.o

ALLOBJ=$(IPOBJ) $(RTMONOBJ)
TARGETS=ip rtmon

all: $(TARGETS)

ip: $(IPOBJ) $(LIBNETLINK) $(LIBUTIL)

rtmon: $(RTMONOBJ) $(LIBNETLINK)

install: all
	install -m 0755 -s $(TARGETS) $(DESTDIR)$(SBINDIR)
	install -m 0755 routel routef $(DESTDIR)$(SBINDIR)

clean:
	rm -f $(ALLOBJ) $(TARGETS)

