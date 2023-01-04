#
# @(#)Makefile 1.1 86/09/25 SMI; from S5R2 1.7
#
INS = install
FLG=
CFLAGS =-O -DFLEXNAMES
FILES = Makefile README cflow.sh dag.c lpfx.c nmf.c flip.c
ALL = cflow.sh dag lpfx nmf flip
LINT = ../../5bin/lint
MIP = ../../5bin/lint/mip
BIN = /usr/bin
LIB = /usr/lib

all:	$(ALL)

dag:	dag.c
	$(CC) $(CFLAGS) $(LDFLAGS) dag.c -o dag

lpfx:	lpfx.c $(LINT)/lerror.h $(LINT)/lmanifest $(LINT)/lpass2.h \
		$(MIP)/manifest
	$(CC) $(CFLAGS) -I$(LINT) -I$(MIP) $(IFLAGS) lpfx.c -o lpfx

nmf:	nmf.c
	$(CC) $(CFLAGS) nmf.c $(LDFLAGS) -o nmf

flip:	flip.c
	$(CC) $(CFLAGS) flip.c $(LDFLAGS) -o flip

install:	$(ALL)
	$(INS) -c cflow.sh $(DESTDIR)$(BIN)/cflow
	$(INS) -s dag $(DESTDIR)$(LIB)/dag
	$(INS) -s lpfx $(DESTDIR)$(LIB)/lpfx
	$(INS) -s nmf $(DESTDIR)$(LIB)/nmf
	$(INS) -s flip $(DESTDIR)$(LIB)/flip

clean:
	-rm -f *.o a.out make.out core
	-rm -f $(ALL)
