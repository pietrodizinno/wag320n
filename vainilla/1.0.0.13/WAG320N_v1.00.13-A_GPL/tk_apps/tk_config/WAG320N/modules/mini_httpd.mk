include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN = mini_httpd
export OBJ = mini_httpd.o match.o tdate_parse.o matrixssl_helper.o https.o shutils.o
export HDR = mime_encodings.h mime_types.h
export PEM = mini_httpd.pem
export SLIB =
export DLIB = 

export CFLAGS = -DUSE_SSL -DMATRIX_SSL -O2
ifeq ($(LCDD),1)
CFLAGS += -D_LCDD_
endif
ifeq ($(HNAP),1)
CFLAGS += -DSUPPORT_HNAP
ifeq ($(YAHOO_ODM),1)
CFLAGS += -DYAHOO_ODM
endif
endif
export LDFLAGS = -L$(TK_APPS_PATH)/libs/libnv -L$(TK_APPS_PATH)/matrixssl-1-8-8-open/src  -L $(TK_APPS_PATH)/libs/libscfg -L $(TK_APPS_PATH)/../target/lib

export ARFLAGS =
export STFLAGS =

export INC = -I$(TK_APPS_PATH)/libs/libnv -I$(TK_APPS_PATH)/matrixssl-1-8-8-open -I$(TK_APPS_PATH)/matrixssl-1-8-8-open/src
export KER_INC =
export LDLIBS = -lmatrixssl -lscfg -lnv -lpthread

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
