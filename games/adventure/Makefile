#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.3 85/09/07
#

DESTDIR=
CFILES=	done.c init.c io.c main.c save.c subr.c vocab.c wizard.c
OBJS=	done.o init.o io.o main.o save.o subr.o vocab.o wizard.o
HFILES = hdr.h
SRCS = $(CFILES) $(HFILES) glorkz
CFLAGS=	-O
LDFLAGS=-n	# should be -i on small machines, -n on a vax

advent:	a.out
	rm -f advent
	echo y | ./a.out

a.out: ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} ${OBJS}

save.o:	save.c	# vax C optimizer bug - infinite loop
	cc -c save.c

# always recomple okplay
okplay.o: /tmp

install: advent
	install -s advent ${DESTDIR}/usr/games/adventure
#	cp okplay /usr/games/adventure
#	cp advsaver /usr/games/lib
#	dd if=/usr/games/lib/adventure of=/usr/games/lib/advheader bs=1 count=16
#	chmod 711 /usr/games/lib
#	chmod 700 /usr/games/lib/adventure
#	chmod 755 /usr/games/lib/advheader
#	chmod 755 /usr/games/lib/advsaver
#	chmod 4711 /usr/games/adventure
#	chown games /usr/games/adventure /usr/games/lib/adventure /usr/games/lib/advheader /usr/games/lib/advsaver
#	rm advent a.out okplay 

okplay: okplay.c
	${CC} ${CFLAGS} -o okplay okplay.c

advsaver: advsaver.s
	${AS} -o advsaver advsaver.s

lint:
	lint $(CFILES) >& lint.out

clean:
	rm -f ${OBJS} a.out advent core errs

sources: $(SRCS)

$(SRCS):
	sccs get $@
