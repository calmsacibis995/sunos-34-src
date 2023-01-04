#
# @(#)Makefile 1.1 86/09/25 SMI;
#
DESTDIR=
CFLAGS=-O
OBJS= etherxdr.o klm_prot.o mountxdr.o nlm_prot.o sm_inter.o \
	rex_xdr.o rquotaxdr.o rstatxdr.o rusersxdr.o rwallxdr.o \
	sprayxdr.o util.o yppasswdxdr.o
HDRS= ether.h klm_prot.h mount.h nlm_prot.h rex.h \
	rquota.h rstat.h rusers.h rwall.h sm_inter.h spray.h yppasswd.h

librpcsvc.a: $(OBJS)
	ar cr librpcsvc.a $(OBJS)

install:	librpcsvc.a installhdrs
	install -c -m 644 librpcsvc.a $(DESTDIR)/usr/lib
	ranlib ${DESTDIR}/usr/lib/librpcsvc.a

installhdrs: $(HDRS)
	for i in $(HDRS); do \
		(install -c -m 644 $$i ${DESTDIR}/usr/include/rpcsvc) done
clean:
	rm -f *.o librpcsvc.a
