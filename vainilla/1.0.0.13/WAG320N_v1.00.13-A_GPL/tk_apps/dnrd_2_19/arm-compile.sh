#!/bin/bash -x
#Cross-compiles GMP using mips compiler

CROSS_COMPILE=arm-linux-uclibc-

if [ ! -e ./Makefile ];then
    ./configure $* \
        --target=arm-linux-uclibc --host=arm-linux-uclibc --build=i686-pc-linux-gnu \
        CXX=${CROSS_COMPILE}g++ \
        CC=${CROSS_COMPILE}gcc \
        LD=${CROSS_COMPILE}ld \
        RANLIB=${CROSS_COMPILE}ranlib \
        NM=${CROSS_COMPILE}nm \
        AR=${CROSS_COMPILE}ar \
        AS=${CROSS_COMPILE}as \
        STRIP=${CROSS_COMPILE}strip \
        DLLTOOL=${CROSS_COMPILE}dlltool \
        OBJDUMP=${CROSS_COMPILE}objdump;
fi
