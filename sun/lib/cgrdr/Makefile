#   @(#)Makefile 1.3 87/01/09 SMI
#
# Makefile for the cg front ends (cgrdr.o and onepass.o) 
# cgrdr.o reads an ir archive,
# build pcc trees and hand them off to the code generator. 
# It is linked with pcc objects to build cg.
# onepass.o  is loaded in f77pass1 to build a onepass compiler
# it transforms iropt ir to pcc trees and calls codegen
# note that sources depend either on iropt headers or pcc headers but not both

DBX=
IR = /usr/src/sun/usr.lib/iropt
M = ../mip
P = ../pcc

CPP=	/lib/cpp
CPPFLAGS= -I$(IR) -I$M -I$P
CFLAGS= -O $(DBX) $(CPPFLAGS)

IR_HDRS = pcc_defines.h cg_ir.h $(IR)/ir_common.h $(IR)/opdescr.h
PCC_HDRS = $M/cpass2.h $M/mip.h $P/machdep.h $P/machdep2.h
HDRS = ${IR_HDRS} ${PCC_HDRS}

SRCS =	do_ir_archive.c read_ir.c debug.c misc.c pcc.c pccfmt.c cg_main.c \
	intr_map.c  rewrite.c onepass_init.c onepass_proc.c \
	goff.c

OBJS =	do_ir_archive.o read_ir.o debug.o misc.o pcc.o pccfmt.o cg_main.o \
	intr_map.o  rewrite.o onepass_init.o onepass_proc.o

CGRDR_OBJS = do_ir_archive.o read_ir.o debug.o misc.o pcc.o pccfmt.o cg_main.o \
	intr_map.o rewrite.o 

ONEPASS_OBJS = misc.o pcc.o pccfmt.o intr_map.o rewrite.o \
	onepass_init.o onepass_proc.o 

PCC_OBJS = \
	../pcc/freader.o ../pcc/fallo.o ../pcc/fopmatch.o ../pcc/fmatch.o \
	../pcc/ftable.o ../pcc/forder.o ../pcc/fbound.o ../pcc/flocal2.o \
	../pcc/fcomm2.o ../pcc/ffloat2.o ../pcc/fflags2.o ../pcc/foptim2.o \
	../pcc/futil2.o ../pcc/fsu.o 

.DEFAULT:
	sccs get -G$@ $@

cgrdr.o:	$(CGRDR_OBJS)
	ld -r -o cgrdr.o $(CGRDR_OBJS)
	cp cgrdr.o $P

onepass.o : goff $(ONEPASS_OBJS) pcc_objs
	rm -f a.out
	ld -r -d $(ONEPASS_OBJS) $(PCC_OBJS)
	./goff a.out
	mv -f a.out onepass.o

clean:
	rm -f *.o

goff: goff.c
	cc -o $@ goff.c

$(OBJS):
	$(CC) -c $(CFLAGS) $*.c

pcc_objs: FRC
	l=`echo $(PCC_OBJS) | sed -e 's/\.\.\/pcc\///g'`;  cd $P; make $$l

FRC:

depend: $(SRCS) $(HDRS)
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CPP) -M $(CPPFLAGS) $$i | \
		sed -e 's,$(IR),$$(IR),g' >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

do_ir_archive.o: do_ir_archive.c
do_ir_archive.o: ./cg_ir.h
do_ir_archive.o: $(IR)/ir_common.h
do_ir_archive.o: $(IR)/opdescr.h
do_ir_archive.o: /usr/include/stdio.h
do_ir_archive.o: /usr/include/sys/file.h
do_ir_archive.o: /usr/include/sys/fcntl.h
read_ir.o: read_ir.c
read_ir.o: ./cg_ir.h
read_ir.o: $(IR)/ir_common.h
read_ir.o: /usr/include/stdio.h
read_ir.o: /usr/include/ctype.h
read_ir.o: /usr/include/sys/types.h
read_ir.o: /usr/include/sys/sysmacros.h
read_ir.o: /usr/include/sys/stat.h
read_ir.o: /usr/include/sys/uio.h
debug.o: debug.c
debug.o: ./cg_ir.h
debug.o: $(IR)/ir_common.h
debug.o: /usr/include/stdio.h
misc.o: misc.c
misc.o: ./cg_ir.h
misc.o: $(IR)/ir_common.h
misc.o: /usr/include/stdio.h
pcc.o: pcc.c
pcc.o: ./cg_ir.h
pcc.o: $(IR)/ir_common.h
pcc.o: ./pcc_defines.h
pcc.o: /usr/include/stdio.h
pccfmt.o: pccfmt.c
pccfmt.o: ../mip/cpass2.h
pccfmt.o: ../pcc/machdep.h
pccfmt.o: ../pcc/machdep2.h
pccfmt.o: ../mip/mip.h
pccfmt.o: /usr/include/stdio.h
cg_main.o: cg_main.c
cg_main.o: ../mip/cpass2.h
cg_main.o: ../pcc/machdep.h
cg_main.o: ../pcc/machdep2.h
cg_main.o: ../mip/mip.h
cg_main.o: /usr/include/stdio.h
intr_map.o: intr_map.c
rewrite.o: rewrite.c
rewrite.o: ./cg_ir.h
rewrite.o: $(IR)/ir_common.h
rewrite.o: ./pcc_defines.h
rewrite.o: /usr/include/stdio.h
onepass_init.o: onepass_init.c
onepass_init.o: ../mip/cpass2.h
onepass_init.o: ../pcc/machdep.h
onepass_init.o: ../pcc/machdep2.h
onepass_init.o: ../mip/mip.h
onepass_init.o: /usr/include/stdio.h
onepass_proc.o: onepass_proc.c
onepass_proc.o: ./cg_ir.h
onepass_proc.o: $(IR)/ir_common.h
onepass_proc.o: $(IR)/opdescr.h
onepass_proc.o: /usr/include/stdio.h
onepass_proc.o: /usr/include/sys/file.h
onepass_proc.o: /usr/include/sys/fcntl.h
goff.o: goff.c
goff.o: /usr/include/sys/types.h
goff.o: /usr/include/sys/sysmacros.h
goff.o: /usr/include/stdio.h
goff.o: /usr/include/a.out.h
goff.o: /usr/include/sys/exec.h
goff.o: /usr/include/stab.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
