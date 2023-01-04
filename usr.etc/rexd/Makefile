#
# @(#)Makefile 1.1 86/09/25 Copyright Sun Micro 1986
#
# Makefile for rex - the remote execution server

SRC = rexd.c mount_nfs.c on.c where.c unix_login.c 
CFLAGS = -O -I/usr/include/rpcsvc
LD = cc
LIB = -lrpcsvc

all: under where on rexd

rexd: rexd.o mount_nfs.o unix_login.o
	$(LD) -o rexd rexd.o mount_nfs.o unix_login.o $(LIB)

on: on.o where.o
	$(LD) -o on on.o where.o $(LIB)

under: under.o mount_nfs.o
	$(LD) -o under under.o mount_nfs.o $(LIB)

where: where_main.o where.o
	$(LD) -o where where_main.o where.o $(LIB)

tags: $(SRC)
	ctags $(SRC)

clean:
	rm -f *.o rexd where under on a.out *.BAK *.CKP

install: rexd on
	install -s rexd ${DESTDIR}/usr/etc/rpc.rexd
	install -s on ${DESTDIR}/usr/bin/on
