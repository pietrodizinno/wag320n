cmd_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.o := /opt/toolchains/uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin/mips-linux-uclibc-gcc -Wp,-MD,/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/.boardparms.o.d  -nostdinc -isystem /opt/toolchains/uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin/../lib/gcc/mips-linux-uclibc/3.4.2/include -D__KERNEL__ -Iinclude  -include include/linux/autoconf.h -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Os  -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -ffreestanding  -march=mips32 -Wa,-mips32 -Wa,--trap -Iinclude/asm-mips/mach-bcm963xx -Iinclude/asm-mips/mach-generic -fomit-frame-pointer  -Wdeclaration-after-statement  -DCONFIG_BCM96358 -I/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/include/bcm963xx   -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(boardparms)"  -D"KBUILD_MODNAME=KBUILD_STR(boardparms)" -c -o /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.o /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.c

deps_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.o := \
  /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.c \
    $(wildcard include/config/bcm96338.h) \
    $(wildcard include/config/mdio.h) \
    $(wildcard include/config/mdio/pseudo/phy.h) \
    $(wildcard include/config/bcm96348.h) \
    $(wildcard include/config/spi/ssb/0.h) \
    $(wildcard include/config/spi/ssb/1.h) \
    $(wildcard include/config/bcm96358.h) \
    $(wildcard include/config/bcm96368.h) \
    $(wildcard include/config/mmap.h) \
    $(wildcard include/config/bcm96816.h) \
  /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/include/bcm963xx/boardparms.h \
    $(wildcard include/config/spi/ssb/2.h) \
    $(wildcard include/config/spi/ssb/3.h) \

/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.o: $(deps_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.o)

$(deps_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/boardparms/bcm963xx/boardparms.o):
