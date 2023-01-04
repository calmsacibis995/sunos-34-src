#
#       Makefile     1.1     86/09/24
#

SRC=	ypprot_err.c yp_bind.c yperr_string.c \
	yp_enum.c yp_match.c ypxdr.c \
	yp_all.c yp_master.c yp_order.c ypv1_xdr.c ypmaint_xdr.c
OBJS=	ypprot_err.o yp_bind.o yperr_string.o \
	yp_enum.o yp_match.o ypxdr.o \
	yp_all.o yp_master.o yp_order.o ypv1_xdr.o ypmaint_xdr.o
INC=	yp_prot.h ypclnt.h ypv1_prot.h

CFLAGS= -O
# CFLAGS= -g

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

yplib yplib_p yplibs5 yplibs5_p: ${OBJS}
	@echo "building profiled Sys5 yplib"
	@cd profiled-sys5; ar cru ../yplibs5_p ${OBJS}
	@echo "building normal Sys5 yplib"
	@cd sys5; ar cru ../yplibs5 ${OBJS}
	@echo "building profiled yplib"
	@cd profiled; ar cru ../yplib_p ${OBJS}
	@echo "building normal yplib"
	@ar cru yplib ${OBJS}

install:
	-mkdir ${DESTDIR}/usr/include/rpcsvc && \
		chown bin ${DESTDIR}/usr/include/rpcsvc && \
		chmod 755 ${DESTDIR}/usr/include/rpcsvc
	-for i in $(INC); do \
		(install -c -m 644 $$i ${DESTDIR}/usr/include/rpcsvc) done

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
	rm -f yplib yplib_p yplibs5 yplibs5_p
	rm -f linted made $(OBJS) profiled/*.o sys5/*.o profiled-sys5/*.o
