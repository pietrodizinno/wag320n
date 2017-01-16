include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk

TK_LIBS =
TK_LIBS += libs
TK_LIBS += linux-atm-2.4.0
ifeq ($(HTTPS), 1)
	TK_LIBS += matrixssl-1-8-8-open/src
endif

TK_KMOD =
TK_KMOD += push_button

TK_APPS =
TK_APPS += busybox
TK_APPS += dhcp-forwarder
TK_APPS += ez-ipupdate-3.0.11b8
TK_APPS += iproute2-2.6.8
TK_APPS += iptables-1.3.5
TK_APPS += ftp
TK_APPS += mini_httpd-1.17beta1
TK_APPS += miniupnpd-igd
TK_APPS += nbtscan-1.5.1a
TK_APPS += netkit-routedv2-0.1
TK_APPS += ppp-2.4.1
TK_APPS += pvc2684ctl
TK_APPS += syslogd
TK_APPS += udhcp-0.9.7
ifeq ($(WIFI), 1)
ifeq ($(WIFI_CHIP), BCM)
	TK_APPS += wlctl
	TK_APPS += nas
ifeq ($(WPS), 1)
	TK_APPS += wps
	TK_APPS += wsc_monitor
endif
endif
endif
TK_APPS += vlan
TK_APPS += pptp-linux-1.3.1
ifeq ($(QVPN), 1)
	TK_APPS += qvpn_server
	TK_APPS += openssl-0.9.7d  #qvpn need it	
endif
ifeq ($(VPN), 1)
	TK_APPS += openswan-2.4.4
endif

#For Storage
ifeq ($(USB_STORAGE), 1)
	TK_APPS += mkdosfs-2.8
	TK_APPS += util-linux-2.10m/fdisk
	TK_APPS += samba-3.0.22
	TK_APPS += inetd
	TK_APPS += bftpd-1.0.22
endif
TK_APPS += ebtables

TK_APPS += dnrd_2_19
export TK_MODULES := $(TK_LIBS) $(TK_KMOD) $(TK_APPS)
