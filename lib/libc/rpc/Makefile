#
# 	Makefile	1.1	86/09/24
#
DESTDIR=
#ifdef	KERNEL
DISTUSER=distuser
DISTKERN=distkern
#endif

SRC=	auth_none.c auth_unix.c authunix_prot.c \
	clnt_perror.c clnt_raw.c clnt_simple.c clnt_tcp.c clnt_udp.c get_myaddress.c\
	pmap_clnt.c pmap_getmaps.c pmap_getport.c pmap_prot.c pmap_prot2.c pmap_rmt.c rpc_prot.c rpc_callmsg.c \
	svc.c svc_auth.c svc_auth_unix.c svc_raw.c svc_simple.c \
	svc_tcp.c svc_udp.c xdr.c xdr_array.c xdr_float.c xdr_mem.c xdr_rec.c xdr_reference.c xdr_stdio.c

#ifdef KERNEL
KSRC=	auth_kern.c clnt_kudp.c kern_syscall.c pmap_kgetport.c \
	subr_kudp.c svc_kudp.c xdr_mbuf.c
KALL=	auth.h auth_unix.h clnt.h rpc_msg.h svc.h svc_auth.h\
	types.h xdr.h auth_none.c auth_unix.c authunix_prot.c\
	clnt_perror.c pmap_getport.c pmap_prot.c \
	rpc_prot.c rpc_callmsg.c svc.c svc_auth.c svc_auth_unix.c xdr.c xdr_array.c xdr_float.c xdr_mem.c xdr_reference.c \
	$(KSRC)

#endif
OBJ=	auth_none.o auth_unix.o authunix_prot.o clnt_perror.o clnt_raw.o\
	clnt_simple.o clnt_tcp.o clnt_udp.o get_myaddress.o \
	pmap_clnt.o pmap_getmaps.o pmap_getport.o pmap_prot.o pmap_prot2.o pmap_rmt.o rpc_prot.o rpc_callmsg.o \
	svc.o svc_auth.o svc_auth_unix.o svc_raw.o svc_simple.o \
	svc_tcp.o svc_udp.o xdr.o xdr_array.o xdr_float.o xdr_mem.o xdr_rec.o xdr_reference.o xdr_stdio.o
INC=	auth.h auth_unix.h clnt.h pmap_clnt.h\
	pmap_prot.h rpc.h rpc_msg.h svc.h svc_auth.h types.h xdr.h

# these are the files included by rpc.h
RPCHDRS= auth.h auth_unix.h clnt.h rpc_msg.h svc.h svc_auth.h types.h xdr.h \
	/usr/include/netinet/in.h

# The -I.. option should go away so that all include files come from the
# same place (/usr/include).
CFLAGS= -O -A-R -I..

.c.o:
	${CC} -p -c ${CFLAGS} -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out profiled-sys5/$*.o
	${CC} ${CFLAGS} -c -I/usr/5include $*.c
	${LD} -x -r $*.o
	mv a.out sys5/$*.o
	${CC} -p -c ${CFLAGS} $*.c
	${LD} -x -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	${LD} -x -r $*.o
	mv a.out $*.o

rpclib rpclib_p rpclibs5 rpclibs5_p: ${OBJ}
	@echo "building profiled Sys5 rpclib"
	@cd profiled-sys5; ar cru ../rpclibs5_p ${OBJ}
	@echo "building normal Sys5 rpclib"
	@cd sys5; ar cru ../rpclibs5 ${OBJ}
	@echo "building profiled rpclib"
	@cd profiled; ar cru ../rpclib_p ${OBJ}
	@echo "building normal rpclib"
	@ar cru rpclib ${OBJ}

install: $(INC)
	-mkdir ${DESTDIR}/usr/include/rpc && \
		chown bin ${DESTDIR}/usr/include/rpc && \
		chmod 755 ${DESTDIR}/usr/include/rpc
	-for i in $(INC); do \
		(install -c -m 644 $$i ${DESTDIR}/usr/include/rpc) done

tags: $(SRC) $(KSRC) $(INC)
	ctags -tw $(SRC) $(KSRC) $(INC)

ref: tags
	sed 's,	/.*,,' tags | \
	awk ' { printf("%-26s%-16s%s\n", $$1, $$2, $$3) }' > ref

lint:
	lint -bnuvx $(SRC)

print:
	pr $(INC) $(SRC) $(KSRC) | lpr -Pvp

clean:
	rm -f rpclib rpclib_p rpclibs5 rpclibs5_p
	rm -f linted made $(OBJ) profiled/*.o sys5/*.o profiled-sys5/*.o

#ifdef KERNEL
distuser:
	-mkdir ${DISTUSER}
	@for i in $(SRC) $(INC) ; do \
	    echo $$i; \
	    cp disclaimer ${DISTUSER}/$$i; \
	    chmod +w ${DISTUSER}/$$i; \
	    unifdef.sh -UKERNEL $$i\
		| sed -e "s,./. <> ./,," >> ${DISTUSER}/$$i;\
	done
	@unifdef.sh -UKERNEL Makefile >> ${DISTUSER}/Makefile
	
distkern:
	-mkdir ${DISTKERN}
	@for i in $(KALL) ; do \
	    echo $$i; unifdef.sh -DKERNEL $$i > ${DISTKERN}/$$i; done
#endif

# This line may cause "sccs get rpc.h", which will fail if it is checked out.
# Oh, for a better make depend...
rpc.h: ${RPCHDRS}

# The first sed line converts <rpc/file.h> to file.h, to conform to the
# action of the -I.. option specified in CFLAGS.
# The second sed line converts <anything> to /usr/include/anything, to
# conform to the rest of the cc include conventions.
# This also doesn't handle /usr/5include header files.
# Urrgghhh...
depend:
	@-grep '^#include' $(SRC) | grep -v '\.\./' | \
	sed -e 's,:[^"]*<rpc/\([^"]*\)>.*,: \1,' \
	-e 's,:[^"]*<\([^"]*\)>.*,: /usr/include/\1,' \
	-e 's/\.[cs]:/.o:/' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' >> makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@cp Makefile makefile.bak
	@ed - Makefile < eddep
	@rm eddep makedep makefile.bak



# DO NOT DELETE THIS LINE

auth_none.o: types.h xdr.h auth.h
auth_unix.o: /usr/include/stdio.h /usr/include/sys/time.h types.h xdr.h auth.h
auth_unix.o: auth_unix.h
authunix_prot.o: types.h xdr.h auth.h auth_unix.h
clnt_perror.o: /usr/include/stdio.h types.h auth.h clnt.h
clnt_raw.o: rpc.h /usr/include/sys/time.h
clnt_simple.o: /usr/include/stdio.h rpc.h /usr/include/sys/socket.h
clnt_simple.o: /usr/include/sys/time.h /usr/include/netdb.h
clnt_simple.o: /usr/include/strings.h
clnt_tcp.o: /usr/include/stdio.h rpc.h /usr/include/sys/socket.h
clnt_tcp.o: /usr/include/sys/time.h /usr/include/netdb.h /usr/include/errno.h
clnt_tcp.o: pmap_clnt.h
clnt_udp.o: /usr/include/stdio.h rpc.h /usr/include/sys/socket.h
clnt_udp.o: /usr/include/sys/time.h /usr/include/sys/ioctl.h
clnt_udp.o: /usr/include/netdb.h /usr/include/errno.h pmap_clnt.h
get_myaddress.o: types.h pmap_prot.h /usr/include/sys/socket.h
get_myaddress.o: /usr/include/stdio.h /usr/include/net/if.h
get_myaddress.o: /usr/include/sys/ioctl.h /usr/include/arpa/inet.h
get_myaddress.o: /usr/include/netinet/in.h
pmap_clnt.o: rpc.h pmap_prot.h pmap_clnt.h /usr/include/sys/time.h
pmap_getmaps.o: rpc.h pmap_prot.h pmap_clnt.h /usr/include/sys/socket.h
pmap_getmaps.o: /usr/include/sys/time.h /usr/include/netdb.h
pmap_getmaps.o: /usr/include/stdio.h /usr/include/errno.h /usr/include/net/if.h
pmap_getmaps.o: /usr/include/sys/ioctl.h
pmap_getport.o: /usr/include/netinet/in.h /usr/include/net/if.h rpc.h
pmap_getport.o: pmap_prot.h pmap_clnt.h /usr/include/sys/time.h
pmap_getport.o: /usr/include/sys/socket.h /usr/include/net/if.h
pmap_prot.o: types.h xdr.h pmap_prot.h
pmap_prot2.o: types.h xdr.h pmap_prot.h
pmap_rmt.o: rpc.h pmap_prot.h pmap_clnt.h /usr/include/sys/socket.h
pmap_rmt.o: /usr/include/sys/time.h /usr/include/stdio.h /usr/include/errno.h
pmap_rmt.o: /usr/include/net/if.h /usr/include/sys/ioctl.h
pmap_rmt.o: /usr/include/arpa/inet.h
rpc_prot.o: types.h xdr.h auth.h clnt.h rpc_msg.h /usr/include/netinet/in.h
rpc_prot.o: /usr/include/sys/param.h rpc.h
rpc_callmsg.o: types.h xdr.h auth.h clnt.h rpc_msg.h /usr/include/netinet/in.h
rpc_callmsg.o: /usr/include/sys/param.h rpc.h
svc.o: /usr/include/netinet/in.h types.h xdr.h auth.h clnt.h rpc_msg.h svc.h
svc.o: svc_auth.h /usr/include/sys/errno.h /usr/include/sys/time.h rpc.h
svc.o: pmap_clnt.h
svc_auth.o: /usr/include/netinet/in.h types.h xdr.h auth.h clnt.h rpc_msg.h
svc_auth.o: svc.h svc_auth.h rpc.h
svc_auth_unix.o: /usr/include/netinet/in.h types.h xdr.h auth.h clnt.h
svc_auth_unix.o: rpc_msg.h svc.h auth_unix.h svc_auth.h /usr/include/stdio.h
svc_auth_unix.o: /usr/include/sys/time.h rpc.h
svc_raw.o: rpc.h
svc_simple.o: /usr/include/stdio.h rpc.h /usr/include/sys/socket.h
svc_simple.o: /usr/include/sys/time.h /usr/include/netdb.h
svc_tcp.o: /usr/include/stdio.h rpc.h /usr/include/sys/socket.h
svc_tcp.o: /usr/include/sys/time.h /usr/include/errno.h
svc_udp.o: /usr/include/stdio.h rpc.h /usr/include/sys/socket.h
svc_udp.o: /usr/include/errno.h
xdr.o: /usr/include/stdio.h types.h xdr.h
xdr_array.o: /usr/include/stdio.h types.h xdr.h
xdr_float.o: /usr/include/stdio.h types.h xdr.h
xdr_mem.o: types.h xdr.h /usr/include/netinet/in.h
xdr_rec.o: /usr/include/stdio.h types.h xdr.h /usr/include/sys/time.h
xdr_rec.o: /usr/include/netinet/in.h
xdr_reference.o: /usr/include/stdio.h types.h xdr.h
xdr_stdio.o: types.h /usr/include/stdio.h xdr.h
