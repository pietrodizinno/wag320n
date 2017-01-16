ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/../..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/iproute2.mk

UTILOBJ=utils.o rt_names.o ll_types.o ll_proto.o ll_addr.o inet_proto.o

NLOBJ=ll_map.o libnetlink.o

all: libnetlink.a libutil.a

libnetlink.a: $(NLOBJ)
	$(AR) rcs $@ $(NLOBJ)

libutil.a: $(UTILOBJ) $(ADDLIB)
	$(AR) rcs $@ $(UTILOBJ) $(ADDLIB)

install:

clean:
	rm -f $(NLOBJ) $(UTILOBJ) $(ADDLIB) libnetlink.a libutil.a

