cp -f bcmdrivers/built-in.o.bak bcmdrivers/built-in.o
cp -f shared/opensource/boardparms/bcm963xx/built-in.o.bak shared/opensource/boardparms/bcm963xx/built-in.o
cp -f shared/opensource/flash/built-in.o.bak shared/opensource/flash/built-in.o
cp -f targets/WAG320N/WAG320N.A targets/WAG320N/WAG320N
make PROFILE=WAG320N kernel_wag320n
cp -f targets/WAG320N/vmlinux.lz ../kernel/vmlinux.lz.a
