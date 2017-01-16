cmd_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.o := /opt/toolchains/uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin/mips-linux-uclibc-gcc -Wp,-MD,/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/.flash_api.o.d  -nostdinc -isystem /opt/toolchains/uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin/../lib/gcc/mips-linux-uclibc/3.4.2/include -D__KERNEL__ -Iinclude  -include include/linux/autoconf.h -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Os  -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -ffreestanding  -march=mips32 -Wa,-mips32 -Wa,--trap -Iinclude/asm-mips/mach-bcm963xx -Iinclude/asm-mips/mach-generic -fomit-frame-pointer  -Wdeclaration-after-statement  -DCONFIG_BCM96358 -I/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/bcmdrivers/opensource/include/bcm963xx -I/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/include/bcm963xx -DINC_CFI_FLASH_DRIVER=1 -DINC_SPI_FLASH_DRIVER=1   -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(flash_api)"  -D"KBUILD_MODNAME=KBUILD_STR(flash_api)" -c -o /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.o /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.c

deps_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.o := \
  /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.c \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/numa.h) \
  /opt/toolchains/uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin/../lib/gcc/mips-linux-uclibc/3.4.2/include/stdarg.h \
  include/linux/linkage.h \
  include/asm/linkage.h \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/enable/must/check.h) \
  include/linux/compiler-gcc3.h \
  include/linux/compiler-gcc.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbd.h) \
    $(wildcard include/config/lsf.h) \
    $(wildcard include/config/resources/64bit.h) \
  include/linux/posix_types.h \
  include/asm/posix_types.h \
  include/asm/sgidefs.h \
  include/asm/types.h \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/64bit/phys/addr.h) \
    $(wildcard include/config/64bit.h) \
  include/linux/bitops.h \
  include/asm/bitops.h \
    $(wildcard include/config/cpu/mipsr2.h) \
    $(wildcard include/config/cpu/mips32.h) \
    $(wildcard include/config/cpu/mips64.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
    $(wildcard include/config/x86.h) \
  include/asm/irqflags.h \
    $(wildcard include/config/mips/mt/smtc.h) \
    $(wildcard include/config/irq/cpu.h) \
    $(wildcard include/config/mips/mt/smtc/instant/replay.h) \
  include/asm/hazards.h \
    $(wildcard include/config/cpu/r10000.h) \
    $(wildcard include/config/cpu/rm9000.h) \
    $(wildcard include/config/cpu/sb1.h) \
  include/asm/barrier.h \
    $(wildcard include/config/cpu/has/sync.h) \
    $(wildcard include/config/cpu/has/wb.h) \
    $(wildcard include/config/weak/ordering.h) \
    $(wildcard include/config/smp.h) \
  include/asm/bug.h \
    $(wildcard include/config/bug.h) \
  include/asm/break.h \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  include/asm/byteorder.h \
    $(wildcard include/config/cpu/mips64/r2.h) \
  include/linux/byteorder/big_endian.h \
  include/linux/byteorder/swab.h \
  include/linux/byteorder/generic.h \
  include/asm/cpu-features.h \
    $(wildcard include/config/32bit.h) \
    $(wildcard include/config/cpu/mipsr2/irq/vi.h) \
    $(wildcard include/config/cpu/mipsr2/irq/ei.h) \
  include/asm/cpu.h \
    $(wildcard include/config/mips/brcm.h) \
  include/asm/cpu-info.h \
    $(wildcard include/config/sgi/ip27.h) \
    $(wildcard include/config/mips/mt.h) \
  include/asm/cache.h \
    $(wildcard include/config/mips/l1/cache/shift.h) \
  include/asm-mips/mach-generic/kmalloc.h \
    $(wildcard include/config/dma/coherent.h) \
  include/asm-mips/mach-bcm963xx/cpu-feature-overrides.h \
    $(wildcard include/config/bcm96358.h) \
    $(wildcard include/config/bcm96368.h) \
    $(wildcard include/config/bcm96816.h) \
  include/asm/war.h \
    $(wildcard include/config/sgi/ip22.h) \
    $(wildcard include/config/sni/rm.h) \
    $(wildcard include/config/cpu/r5432.h) \
    $(wildcard include/config/sb1/pass/1/workarounds.h) \
    $(wildcard include/config/sb1/pass/2/workarounds.h) \
    $(wildcard include/config/mips/malta.h) \
    $(wildcard include/config/mips/atlas.h) \
    $(wildcard include/config/mips/sead.h) \
    $(wildcard include/config/cpu/tx49xx.h) \
    $(wildcard include/config/momenco/jaguar/atx.h) \
    $(wildcard include/config/pmc/yosemite.h) \
    $(wildcard include/config/basler/excite.h) \
    $(wildcard include/config/momenco/ocelot/3.h) \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/ffz.h \
  include/asm-generic/bitops/find.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/ext2-non-atomic.h \
  include/asm-generic/bitops/le.h \
  include/asm-generic/bitops/ext2-atomic.h \
  include/asm-generic/bitops/minix.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/bcmdrivers/opensource/include/bcm963xx/bcmtypes.h \
  /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/include/bcm963xx/bcm_hwdefs.h \
    $(wildcard include/config/16m/flash.h) \
    $(wildcard include/config/8m/flash.h) \
    $(wildcard include/config/bcm96338.h) \
    $(wildcard include/config/brcm/ikos.h) \
  /home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/include/bcm963xx/flash_api.h \

/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.o: $(deps_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.o)

$(deps_/home/fredfu/projects/WAG320N_v1_00_12_GPL/bcm963xx_4.02L.01/shared/opensource/flash/flash_api.o):
