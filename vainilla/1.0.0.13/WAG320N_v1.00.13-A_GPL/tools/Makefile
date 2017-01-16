UCLIB = uClibc-0.9.19

all: make_links #build_lib

build_lib:mipsel-linux-uclibc
	$(MAKE) -C $(UCLIB)
	$(MAKE) -C $(UCLIB) install_target	
#	cp -a /opt/mips-linux-uclibc/lib/* ../target/lib

mipsel-linux-uclibc:
	$(MAKE) -C $(UCLIB)
	$(MAKE) -C $(UCLIB) install

clean:
#	$(MAKE) -C makeimage clean
#	$(MAKE) -C $(UCLIB) clean
#	rm -rf mipsel-linux-uclibc

make_links:
	./mklnks
