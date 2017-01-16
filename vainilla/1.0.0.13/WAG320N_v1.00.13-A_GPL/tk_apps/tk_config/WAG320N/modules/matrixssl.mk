include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/feature.mk
include $(TK_APPS_PATH)/tk_config/$(TK_PROJECT_NAME)/apps_compile.mk

export STRIP_ENABLE = 1

export BIN =
export PEM = certSrv.pem CAcertSrv.pem
export OBJ =
OBJ += cipherSuite.o
OBJ += matrixSsl.o
OBJ += sslDecode.o
OBJ += sslEncode.o
OBJ += sslv3.o
OBJ += sslSocket.o
OBJ += os/debug.o
OBJ += os/linux/linux.o
OBJ += crypto/peersec/arc4.o
OBJ += crypto/peersec/base64.o
OBJ += crypto/peersec/des3.o
OBJ += crypto/peersec/md5.o
OBJ += crypto/peersec/md2.o
OBJ += crypto/peersec/mpi.o
OBJ += crypto/peersec/rsa.o
OBJ += crypto/peersec/sha1.o
OBJ += pki/asn1.o
OBJ += pki/rsaPki.o
OBJ += pki/x509.o

export SLIB = libmatrixsslstatic.a
export DLIB = libmatrixssl.so

export CFLAGS = -Os -fPIC -DLINUX
export LDFLAGS = -shared
export ARFLAGS = -rcuv
export STFLAGS =

export INC = -I./
export KER_INC =
export LDLIBS = -lc -lpthread

export LINK = $(CC)

export INSTALL = install
export INSTALL_DIR = $(TK_INSTALL_PATH)/usr/sbin
export INSTALL_LIB_DIR = $(TK_INSTALL_PATH)/lib