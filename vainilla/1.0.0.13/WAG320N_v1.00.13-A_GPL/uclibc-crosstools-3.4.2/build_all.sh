#/bin/sh

PWD_DIR=`pwd`
PREFIX_DIR=$PWD/BUILD

#Build binutils
function build_binutils()
{
	mkdir -p $PREFIX_DIR/binutils-2.15.94.0.2.2
	cd $PREFIX_DIR/binutils-2.15.94.0.2.2
	../../binutils-2.15.94.0.2.2/configure --prefix=$PREFIX_DIR/uclibc-crosstools-3.4.2 --target=mips-linux-uclibc --build=mips-linux --host=mips-linux --enable-target-optspace  --mandir=$PREFIX_DIR/uclibc-crosstools-3.4.2/man --with-arch=mips32
	make
	make install
	cd ../..
}

#Build sstrip
function build_sstrip()
{
	cd sstrip
	make
	cp -af sstrip $PREFIX_DIR/uclibc-crosstools-3.4.2/bin
	cd ..
}

#Build GDB
function build_gdb()
{
	mkdir -p $PREFIX_DIR/gdb-6.3
	cd $PREFIX_DIR/gdb-6.3
	../../gdb-6.3/configure --host=mips-linux --target=mips-linux-uclibc
	make
	cp -af gdb/gdb $PREFIX_DIR/uclibc-crosstools-3.4.2/bin/mips-linux-uclibc-gdb
	cp -af gdb/gdbtui $PREFIX_DIR/uclibc-crosstools-3.4.2/bin/mips-linux-uclibc-gdbtui
	cd ../..
}

#Copy Kernel header files
function copy_kernel_header()
{
	mkdir -p $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/
	cp -af linux-libc-headers-2.6.11.0/include/asm-mips $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/asm
	cp -af linux-libc-headers-2.6.11.0/include/linux $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/
	cp -af linux-libc-headers-2.6.11.0/include/scsi $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/
}

#Build GLIBC
function build_glibc()
{
	export PATH=$PATH:$PREFIX_DIR/../uclibc-crosstools_gcc-3.4.2_uclibc-20050502/bin
	export CROSS=mips-linux-uclibc-
	cd uClibc-20050502 
	make clean
	cp -af .config.bak .config
	echo "KERNEL_SOURCE=\"$PREFIX_DIR/../uclibc-crosstools_gcc-3.4.2_uclibc-20050502/mips-linux-uclibc/\"" >> .config
	make
	rm -rf $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/*
	cp -rLf include/* $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/
	mkdir -p $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/bits
	cp -af libc/sysdeps/linux/common/bits/* $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/bits/
	cp -af libc/sysdeps/linux/mips/bits/* $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/include/bits/
	mkdir -p $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/lib/
	cp -af lib/* $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc/lib/
	cd $PREFIX_DIR/uclibc-crosstools-3.4.2/
	ln -sf mips-linux-uclibc mips-linux
	cd $PREFIX_DIR/uclibc-crosstools-3.4.2/mips-linux-uclibc
	ln -sf include sys-include
	cd $PREFIX_DIR/../
}

#Build GCC
function build_gcc()
{
	export PATH=$PATH:$PREFIX_DIR/uclibc-crosstools-3.4.2/bin
	mkdir -p $PREFIX_DIR/gcc-3.4.2
	cd $PREFIX_DIR/gcc-3.4.2
	../../gcc-3.4.2/configure --prefix=$PREFIX_DIR/uclibc-crosstools-3.4.2 --target=mips-linux-uclibc --enable-target-optspace --disable-nls --with-gnu-ld --enable-shared --enable-languages=c,c++ --enable-threads --infodir=$PREFIX_DIR/uclibc-crosstools-3.4.2/info --mandir=$PREFIX_DIR/uclibc-crosstools-3.4.2/man --disable-__cxa_atexit --disable-checking --with-arch=mips32
	make
	make install
	cd ../..
}

function main()
{
	mkdir -p $PREFIX_DIR/uclibc-crosstools-3.4.2
	build_binutils
	build_sstrip
	build_gdb
	copy_kernel_header
	build_glibc
	build_gcc
	strip $PREFIX_DIR/uclibc-crosstools-3.4.2/bin/* 2>/dev/null
}

main
