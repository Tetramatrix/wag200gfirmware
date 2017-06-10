export CC=mipsel-uclibc-gcc
export LD=mipsel-uclibc-ld

DIRS= lib arpd

all: $(DIRS)
	make -e -C lib
	make -e -C switch all-am
	make -e -C arpd
	cp -af lib/.libs/libatm.so* ../../../tools/mipsel-linux-uclibc/lib
	cp -af include/atm.h ../../../tools/mipsel-linux-uclibc/include

clean: $(DIRS)
	make -C lib clean
	make -C arpd clean
	make -C switch clean
install:
#	mipsel-uclibc-strip arpd/.libs/*
	mipsel-uclibc-strip arpd/.libs/atmarp
	mipsel-uclibc-strip arpd/.libs/atmarpd
	mipsel-uclibc-strip lib/.libs/libatm.so*
#	cp -af arpd/.libs/* ../../../target/usr/sbin/
	cp -af arpd/.libs/atmarp ../../../target/usr/sbin/
	cp -af arpd/.libs/atmarpd ../../../target/usr/sbin/
	cp -af lib/.libs/libatm.so* ../../../target/lib
