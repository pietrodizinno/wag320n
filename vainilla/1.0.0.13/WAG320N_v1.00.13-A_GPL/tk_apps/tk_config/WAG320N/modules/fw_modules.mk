include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/kernel_compile.mk

export KVER = 2.6
export STRIP_ENABLE = 0

export KERNEL_DIR = $(TK_KERNEL_PATH)

ifeq ($(KVER), 2.4)
	export CFLAGS = -D__KERNEL__ -Wall -Wstrict-prototypes -Wno-trigraphs -Os -fomit-frame-pointer -fno-strict-aliasing -fno-common -fno-pic -pipe -DMODULE -DMODVERSIONS 
	export KER_INC = -I$(KERNEL_DIR)/include/asm/gcc -I$(KERNEL_DIR)/include -include $(KERNEL_DIR)/include/linux/modversions.h
	export KER_MOD =
	       KER_MOD += ip_conntrack_pt.o
	       KER_MOD += ip_nat_pt.o
else ifeq ($(KVER), 2.6)
	export obj-m =
           #obj-m += ipt_DLOG.o
           #obj-m += ipt_LOG.o
           #obj-m += ipt_http_string.o
           #obj-m += ipt_multi_match.o
           #obj-m += ipt_webstr.o
           #obj-m += ipt_psd.o
           #obj-m += ip_conntrack_pt.o
           #obj-m += ipt_string.o
           #obj-m += ipt_stringGET.o
           #obj-m += ipt_stringHEAD.o
           #obj-m += ipt_stringHOST.o
           #obj-m += ipt_unkowntype.o
ifeq ($(YAHOO_ODM), 1)
		   obj-m += ipt_dns.o
		   obj-m += ipt_dns_msq.o
		   obj-m += ipt_httpReturnCode.o
		   obj-m += ipt_yahoo.o
endif
	obj-m += ipt_httpReturnCode.o
	obj-m += ipt_httpMETHOD.o
endif

export STFLAGS = --strip-debug

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/lib/modules

