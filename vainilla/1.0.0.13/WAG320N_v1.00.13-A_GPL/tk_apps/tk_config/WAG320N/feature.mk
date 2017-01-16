
export HW_ID = $(TK_PROJECT_NAME)

export VER = 0.00.01
export MODULE = Linksys
export USB = 0
export FLASH_SIZE = 8M
export RAMDISK_SIZE = 14336
export MULTI_PVC = 0
export COMPANY = 08
export MT_CODE = 0
export ADSL2 = 0
export SNMP = 0
export SIG_MAIL = 0
export QOS = 1
export BCM_WIFI = 0
export IPS = 0
export IGMP_PROXY = 1
export EZCONFIG = 0
export SWITCH = 0
export LLDP = 0
export LCDD = 0
export TMSS = 0
export CA = 0
export GUI_CHANGE_LANG = 1
export L2TP = 0
export PPTP = 0
export BPA = 0
export MANUAL_TIME = 1
export HNAP = 1
export LED = 1
export YAHOO_ODM = 0
export USB_STORAGE = 1
export DLNA_CERTIFICATE = 1
export VPN = 0
export FAST_BRIDGE=1
#export TMICRO = 1
export SIPALG = 1
export TELNET = 0

ifeq ($(VPN), 1)
	export QVPN = 1
endif

export WIFI = 1
export WPS = 1
ifeq ($(WIFI), 1)
#	export WIFI_CHIP = TI
	export WIFI_CHIP = BCM
#	export WIFI_CHIP = MARVELL
#	export WIFI_CHIP = AIRGO
#	export WIFI_CHIP = RALINK
#	export WIFI_CHIP = CONEXANT
endif

export REGION = EU
#export REGION = USA

export IPV6 = 0
ifeq ($(IPV6), 1)
	export RADVD = 1
	export NATPT = 1
endif

export HTTPS = 1
ifeq ($(HTTPS), 1)
	export HTTPS_MODE = MATRIXSSL
#	export HTTPS_MODE = OPENSSL
endif

export ADSL = 1
ifeq ($(ADSL), 1)
#	export ADSL_CHIP = BCM
	export ADSL_CHIP = CNX
#	export ANNEX = A
	export ATM2684 = 1
	ifeq ($(ATM2684), 1)
#		export ATM2684_TOOL = BR2684
#		export ATM2684_TOOL = RT2684
		export ATM2684_TOOL = PVC2684
	endif
endif

