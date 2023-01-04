#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 1.12 83/07/03
#
PSRCS = \
	/usr/include/dumprestor.h dump.h \
	dumpmain.c dumptraverse.c dumptape.c dumpoptr.c dumpitime.c unctime.c

SRCS =  \
	dump.h dumpitime.c \
	dumpmain.c dumpoptr.c dumprmt.c dumprtape.c \
	dumptraverse.c unctime.c

OBJS = dumpitime.o dumpmain.o dumpoptr.o \
	dumptape.o dumprtape.o dumptraverse.o dumprmt.o unctime.o

DFLAGS =
CFLAGS = -O $(DFLAGS)
TOUCH =	touch -cf

all: dump

dump:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o dump

install: all
	install -s dump $(DESTDIR)/etc
	-rm -f $(DESTDIR)/etc/rdump
	ln -s /etc/dump $(DESTDIR)/etc/rdump

clean:
	rm -f *.o dump errs dumprmain.c

lint:
	lint $(DFLAGS) $(SRCS)

print:
	pr $(PSRCS) | lpr

dump.h:
	$(TOUCH) dump.h

depend:
	cp /dev/null x.c
	for i in $(SRCS) ; do \
		(/bin/grep '^#[ 	]*include' x.c $$i | sed \
			-e '/\.\.\/h/d' \
			-e '/\.\.\/ufs/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c/\.o/' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

dump.h: /usr/include/stdio.h
dump.h: /usr/include/ctype.h
dump.h: /usr/include/sys/param.h
dump.h: /usr/include/sys/stat.h
dump.h: /usr/include/sys/time.h
dump.h: /usr/include/sys/vnode.h
dump.h: /usr/include/ufs/inode.h
dump.h: /usr/include/ufs/fs.h
dump.h: /usr/include/dumprestor.h
dump.h: /usr/include/sys/dir.h
dump.h: /usr/include/utmp.h
dump.h: /usr/include/signal.h
dump.h: /usr/include/fstab.h
dumpitime.o: dump.h
dumpitime.o: /usr/include/sys/file.h
dumpmain.o: dump.h
dumpoptr.o: dump.h
dumprmt.o: /usr/include/sys/param.h
dumprmt.o: /usr/include/sys/mtio.h
dumprmt.o: /usr/include/sys/ioctl.h
dumprmt.o: /usr/include/netinet/in.h
dumprmt.o: /usr/include/stdio.h
dumprmt.o: /usr/include/netdb.h
dumprtape.o: dump.h
dumptraverse.o: dump.h
unctime.o: /usr/include/sys/types.h
unctime.o: /usr/include/sys/time.h
unctime.o: /usr/include/stdio.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
