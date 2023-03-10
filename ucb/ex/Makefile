#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 7.12 9/7/85
#
VERSION=3.7
#
# Ex skeletal makefile for VAX VM/Unix 4.2BSD
#
# NB: This makefile doesn't indicate any dependencies on header files.
#
# Ex is very large - this version will not fit on PDP-11's without overlay
# software.  Things that can be turned off to save
# space include LISPCODE (-l flag, showmatch and lisp options), UCVISUAL
# (visual \ nonsense on upper case only terminals), CHDIR (the undocumented
# chdir command.)  CRYPT includes the code to edit encrypted files (the -x
# option, like ed.)  VMUNIX makes ex considerably larger, raising many limits
# and improving speed and simplicity of maintenance.  It is suitable only
# for a VAX or other large machine, and then probably only in a paged system.
#
# Don't define VFORK unless your system has the VFORK system call,
# which is like fork but the two processes have only one data space until the
# child execs. This speeds up ex by saving the memory copy.
#
# If your system expands tabs to 4 spaces you should -DTABS=4 below
#
BINDIR=	/usr/ucb
NBINDIR=/usr/new
LIBDIR=	/usr/lib
FOLD=	${BINDIR}/fold
CTAGS=	${BINDIR}/ctags
XSTR=	${BINDIR}/xstr
DEBUGFLAGS=	-DTRACE -g
NONDEBUGFLAGS=	-O
DEB=	${NONDEBUGFLAGS}	# or ${DEBUGFLAGS} to to debug
CRYPT=	-DCRYPT
OPTIONS= ${CRYPT} -DLISPCODE -DCHDIR -DUCVISUAL -DVFORK -DVMUNIX -DFASTTAG -DTAG_STACK
CFLAGS=	-DTABS=8 ${OPTIONS} ${DEB}
LDFLAGS=	-z		# or -i or -n
TERMLIB=	-ltermcap
MKSTR=	${BINDIR}/mkstr
CXREF=	${BINDIR}/cxref
INCLUDE=/usr/include
PR=	pr
OBJS=	ex.o ex_addr.o ex_cmds.o ex_cmds2.o ex_cmdsub.o \
	ex_data.o ex_extern.o ex_get.o ex_io.o ex_put.o ex_re.o \
	ex_set.o ex_subr.o ex_tagio.o ex_temp.o ex_tty.o ex_unix.o \
	ex_v.o ex_vadj.o ex_vget.o ex_vmain.o ex_voper.o \
	ex_vops.o ex_vops2.o ex_vops3.o ex_vput.o ex_vwind.o \
	printf.o strings.o	# bcopy.o; now get from libc
HDRS=	ex.h ex_argv.h ex_re.h ex_temp.h ex_tty.h ex_tune.h ex_vars.h ex_vis.h
SRC1=	ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c
SRC2=	ex_data.c ex_get.c ex_io.c ex_put.c ex_re.c
SRC3=	ex_set.c ex_subr.c ex_tagio.c ex_temp.c ex_tty.c ex_unix.c
SRC4=	ex_v.c ex_vadj.c ex_vget.c ex_vmain.c ex_voper.c
SRC5=	ex_vops.c ex_vops2.c ex_vops3.c ex_vput.c ex_vwind.c
SRC6=	printf.c bcopy.c expreserve.c exrecover.c
MISC=	makefile READ_ME rofix
VGRIND=	csh /usr/ucb/vgrind
VHDR=	"Ex Version ${VERSION}"

.c.o:
# ifdef VMUNIX
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
# else
#	${MKSTR} - ex${VERSION}strings x $*.c
#	${CC} -E ${CFLAGS} x$*.c | ${XSTR} -c -
#	rm -f x$*.c
# endif
	${CC} ${CFLAGS} -c x.c 
	mv x.o $*.o

all:	a.out exrecover expreserve

a.out: ${OBJS}
	@echo loading a.out
	@${CC} ${LDFLAGS} ${OBJS} ${TERMLIB}

tags:	/tmp
	${CTAGS} -w ex.[hc] ex_*.[hc]

${OBJS}: ${HDRS}

# ex_vars.h:
# 	csh makeoptions ${CFLAGS}

bcopy.o:	bcopy.c
	${CC} -c ${CFLAGS} bcopy.c

# xstr: hands off!
strings.o: strings
	${XSTR}
	${CC} -c -S xs.c
	ed - <rofix xs.s
	${AS} -o strings.o xs.s
	rm xs.s
	
exrecover: exrecover.o
	${CC} ${CFLAGS} exrecover.o ex_extern.o -o exrecover

exrecover.o: exrecover.c
	${CC} ${CFLAGS} -c -O exrecover.c

expreserve: expreserve.o
	${CC} expreserve.o -o expreserve

expreserve.o:
	${CC} ${CFLAGS} -c -O expreserve.c

international: iex.o iex_temp.o iex_io.o
	mv iex.o ex.o
	mv iex_temp.o ex_temp.o
	mv iex_io.o ex_io.o
	make ${MFLAGS} install DESTDIR=${DESTDIR}
	rm ex.o ex_io.o ex_temp.o

iex.o: ex.c
	-rm ex.o
	make ${MFLAGS} "CRYPT=" ex.o
	mv ex.o iex.o	
	
iex_io.o: ex_io.c
	-rm ex_io.o
	make ${MFLAGS} "CRYPT=" ex_io.o
	mv ex_io.o iex_io.o	
	
iex_temp.o: ex_temp.c
	-rm ex_temp.o
	make ${MFLAGS} "CRYPT=" ex_temp.o
	mv ex_temp.o iex_temp.o	
	
clean:
#	If we dont have ex we cant make it so don't rm ex_vars.h
	-rm -f a.out exrecover expreserve strings core errs trace
	-rm -f *.o x*.[cs]

# install a new version for testing in /usr/new
ninstall: a.out
	-rm -f ${DESTDIR}${NBINDIR}/ex ${DESTDIR}${NBINDIR}/vi ${DESTDIR}${NBINDIR}/view
	cp a.out ${DESTDIR}${NBINDIR}/ex
#	-cp ex${VERSION}strings ${LIBDIR}/ex${VERSION}strings
	ln ${DESTDIR}${NBINDIR}/ex ${DESTDIR}${NBINDIR}/vi
	ln ${DESTDIR}${NBINDIR}/ex ${DESTDIR}${NBINDIR}/view
	chmod 1755 ${DESTDIR}${NBINDIR}/ex

# install in standard place (/usr/ucb)
install: a.out exrecover expreserve
	-rm -f ${DESTDIR}/${BINDIR}/ex
	-rm -f ${DESTDIR}/${BINDIR}/vi
	-rm -f ${DESTDIR}/${BINDIR}/view
	-rm -f ${DESTDIR}/${BINDIR}/edit
	-rm -f ${DESTDIR}/${BINDIR}/e
	-rm -f ${DESTDIR}/usr/bin/ex
	install -s -m 1755 a.out ${DESTDIR}/${BINDIR}/ex
#	cp ex${VERSION}strings ${DESTDIR}/${LIBDIR}/ex${VERSION}strings
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/edit
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/e
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/view
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/usr/bin/ex
	chmod 1755 ${DESTDIR}/${BINDIR}/ex
	install -s -m 4755 exrecover ${DESTDIR}/${LIBDIR}/ex${VERSION}recover
	install -s -m 4755 expreserve ${DESTDIR}/${LIBDIR}/ex${VERSION}preserve
# The following line normally fails.  This is OK.
	-mkdir ${DESTDIR}/usr/preserve && chown bin ${DESTDIR}/usr/preserve && \
		chmod 755 ${DESTDIR}/usr/preserve

# move from /usr/new to /usr/ucb
newucb: a.out
	-rm -f ${DESTDIR}/${BINDIR}/ex
	-rm -f ${DESTDIR}/${BINDIR}/vi
	-rm -f ${DESTDIR}/${BINDIR}/edit
	-rm -f ${DESTDIR}/${BINDIR}/e
	-rm -f ${DESTDIR}/usr/bin/ex
	mv ${DESTDIR}/${NBINDIR}/ex ${DESTDIR}/${NBINDIR}/ex
	-rm -f ${DESTDIR}/${NBINDIR}/vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/edit
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/e
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/usr/bin/ex
	chmod 1755 ${DESTDIR}/${BINDIR}/ex

lint:
	lint ${CFLAGS} ex.c ex_?*.c
	lint ${CFLAGS} -u exrecover.c
	lint ${CFLAGS} expreserve.c

print:
	@${PR} READ* BUGS
	@${PR} makefile*
	@${PR} /etc/termcap
	@(size -l a.out ; size *.o) | ${PR} -h sizes
	@${PR} -h errno.h ${INCLUDE}/errno.h
	@${PR} -h setjmp.h ${INCLUDE}/setjmp.h
	@${PR} -h sgtty.h ${INCLUDE}/sgtty.h
	@${PR} -h signal.h ${INCLUDE}/signal.h
	@${PR} -h sys/stat.h ${INCLUDE}/sys/stat.h
	@${PR} -h sys/types.h ${INCLUDE}/sys/types.h
	@ls -ls | ${PR}
	@${CXREF} *.c | ${PR} -h XREF
	@${PR} *.h *.c
vgrind:
	tee index < /dev/null
	${VGRIND} -h ${VHDR} ${HDRS}
	${VGRIND} -h ${VHDR} ${SRC1}
	${VGRIND} -h ${VHDR} ${SRC2}
	${VGRIND} -h ${VHDR} ${SRC3}
	${VGRIND} -h ${VHDR} ${SRC4}
	${VGRIND} -h ${VHDR} ${SRC5}
	${VGRIND} -h ${VHDR} ${SRC6}
	${VGRIND} -n -h ${VHDR} ${MISC}
	${VGRIND} -i -h ${VHDR} index
