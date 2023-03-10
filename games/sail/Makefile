#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 1.10 83/07/02
#
# Sail - Wooden Ships And Iron Men
#
CC= cc
CTAGS= ctags -w
DFILES= driver1.c driver2.c driver3.c grap.c
DOBJS= driver1.o driver2.o driver3.o grap.o
PFILES= version.c player1.c player2.c
POBJS= version.o player1.o player2.o
LFILES= sail.log.c
LOBJS= sail.log.o
COMMONFILES= assorted.c boarders.c game.c globals.c\
	main.c parties.c screen.c machdep.c
COMMONOBJS= assorted.o boarders.o game.o globals.o\
	main.o parties.o screen.o machdep.o
HDRS= externs.h player.h machdep.h
OTHERFILES= Makefile
CTAGFILES= ${DFILES} ${PFILES} ${LFILES} ${COMMONFILES}
JUNKFILES= sail driver sail.log junk arch tags
PLIBS= -lcurses -ltermlib machdep.o
DLIBS=
LLIBS=
SAILMASTER= daemon
SAIL= sail
DRIVER= lib/saildriver
SAIL.LOG= sail.log
SHIPSLOG= lib/saillog
DESTDIR=
TARGET= ${DESTDIR}/usr/games
CFLAGS= -O -DLOGFILE="\"${TARGET}/${SHIPSLOG}\"" 

all: sail driver sail.log

sail: ${COMMONOBJS} ${POBJS}
	cc -O ${COMMONOBJS} ${POBJS} ${PLIBS} -o sail

driver: ${COMMONOBJS} ${DOBJS}
	cc -O ${COMMONOBJS} ${DOBJS} ${DLIBS} -o driver

${DOBJS} ${POBJS} ${LOBJS} ${COMMONOBJS}: externs.h machdep.h

sail.log: ${LOBJS} globals.o
	cc -O ${LOBJS} globals.o -o sail.log

tags: ${CTAGFILES}
	${CTAGS} ${CTAGFILES}

${HDRS} ${CTAGFILES}:
	sccs get $@;

arch:	sail.ar
	ar uv sail.ar ${PFILES} ${COMMONFILES} ${OTHERFILES} \
	${DFILES} ${LFILES} ${HDRS}

driveprofile: ${COMMONOBJS} ${DOBJS}
	cc -i -p ${COMMONOBJS} ${DOBJS} ${CFLAGS} ${DLIBS} -o DRIVE.pro

profile: ${COMMONOBJS} ${POBJS} ${DOBJS}
	cc -i -p ${COMMONOBJS} ${POBJS} ${CFLAGS} ${PLIBS} -o SAIL.pro
	cc -i -p ${COMMONOBJS} ${DOBJS} ${CFLAGS} ${DLIBS} -o DRIVE.pro

clean: 
	rm -f ${DOBJS} ${POBJS} ${LOBJS} ${COMMONOBJS} ${JUNKFILES}

install: sail driver sail.log ${TARGET}/${SHIPSLOG}
	install -s -m 4711 -o ${SAILMASTER} sail ${TARGET}/${SAIL}
	install -s -m 4711 -o ${SAILMASTER} driver ${TARGET}/${DRIVER}
	install -s -m 711 sail.log ${TARGET}/${SAIL.LOG}

${TARGET}/${SHIPSLOG}:
	cp /dev/null ${TARGET}/${SHIPSLOG}
	chown ${SAILMASTER} ${TARGET}/${SHIPSLOG}
	chmod 644 ${TARGET}/${SHIPSLOG}
