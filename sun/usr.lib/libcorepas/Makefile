#
# @(#)Makefile @(#)Makefile 1.1 86/09/25 SMI
#

#   Build libcorepas.a -- Pascal interface routines for SunCore library

DESTDIR =

CFLAGS = -O
PFLAGS = -c -w -g

OBJECTS = \
	batchpas.o credelsegpas.o errorspas.o getvwsurfpas.o init_termpas.o inprimspas.o\
	inqinputpas.o inqprimattpas.o inqsegattpas.o inqtextentpas.o inquirypas.o\
	ndctowldpas.o newframepas.o outprim2pas.o outprim3pas.o polygon2pas.o\
	polygon3pas.o rasterfiopas.o rasterprimpas.o\
	savesegpas.o segatt3pas.o\
	segdefspas.o segmentspas.o setprimattpas.o setsegattpas.o\
	textmarkpas.o view_surfpas.o view_transpas.o xformattpas.o\
	zbufferpas.o

libcorepas.a:	$(OBJECTS)
	ar ru libcorepas.a $(OBJECTS)
	ranlib libcorepas.a

clean:
	rm -f *.o libcorepas.a

install:	libcorepas.a
	install -c -m 644 libcorepas.a ${DESTDIR}/usr/lib
	ranlib ${DESTDIR}/usr/lib/libcorepas.a
	-mkdir ${DESTDIR}/usr/include/pascal && \
		chown bin ${DESTDIR}/usr/include/pascal && \
		chmod 755 ${DESTDIR}/usr/include/pascal
	install -c -m 644 typedefspas.h ${DESTDIR}/usr/include/pascal
	install -c -m 644 usercorepas.h ${DESTDIR}/usr/include/pascal
	install -c -m 644 sunpas.h ${DESTDIR}/usr/include/pascal
	install -c -m 644 devincpas.h ${DESTDIR}/usr/include/pascal

.c.o: ; cc -c $(CFLAGS) $*.c
.p.o: ; pc $(PFLAGS) $*.p
