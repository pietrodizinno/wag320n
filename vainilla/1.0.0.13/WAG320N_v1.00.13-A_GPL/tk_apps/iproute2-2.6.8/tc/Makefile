ifndef TK_APPS_PATH
	export TK_APPS_PATH := $(shell pwd)/../..
	include $(TK_APPS_PATH)/tk_config/project_name.mk
endif

include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/modules/iproute2.mk

TCOBJ= tc.o tc_qdisc.o tc_class.o tc_filter.o tc_util.o \
       m_police.o m_estimator.o m_action.o

include ../Config

TCMODULES :=
TCMODULES += q_fifo.o
TCMODULES += q_sfq.o
TCMODULES += q_red.o
TCMODULES += q_prio.o
TCMODULES += q_tbf.o
TCMODULES += q_cbq.o
TCMODULES += f_rsvp.o
TCMODULES += f_u32.o
TCMODULES += f_route.o
TCMODULES += f_fw.o
TCMODULES += q_dsmark.o
TCMODULES += q_gred.o
TCMODULES += f_tcindex.o
TCMODULES += q_ingress.o
TCMODULES += q_hfsc.o
TCMODULES += q_htb.o

TCOBJ += $(TCMODULES)

TCLIB := tc_core.o
TCLIB += tc_red.o
TCLIB += tc_cbq.o
TCLIB += tc_estimator.o

TCSO :=
TCSO += q_netem.so
ifeq ($(TC_CONFIG_ATM),y)
  TCSO += q_atm.so
endif

LDLIBS += -L. -ltc -lm -ldl
LDFLAGS += -Wl,-export-dynamic

%.so: %.c
	$(CC) $(CFLAGS) -shared -fpic $< -o $@

all: libtc.a tc $(TCSO)

tc: $(TCOBJ) $(LIBNETLINK) $(LIBUTIL) $(TCLIB)

libtc.a: $(TCLIB)
	$(AR) rcs $@ $(TCLIB)

install: all
	mkdir -p $(DESTDIR)/usr/lib/tc
	install -m 0755 -s tc $(DESTDIR)$(SBINDIR)
	for i in $(TCSO); do install -m 755 -s $$i $(DESTDIR)/usr/lib/tc; done

clean:
	rm -f $(TCOBJ) $(TCLIB) libtc.a tc

q_atm.so: q_atm.c
	$(CC) $(CFLAGS) -shared -fpic -o q_atm.so q_atm.c -latm


