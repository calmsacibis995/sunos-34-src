#
# @(#)Makefile 1.1 86/09/25 SMI; from S5R2 1.2
#
INS = install
INSDIR = $(DESTDIR)/usr/bin
CFLAGS = -O -DBSD
FFLAG =
OFILES = atof.o bs.o string.o
IFLAG =

bs:	$(OFILES)
	$(CC) $(LDFLAGS) $(FFLAG) $(IFLAG) -o bs $(OFILES) -lm 

atof.o:	atof.c
bs.o:	bs.c
string.o: string.c

test:
	bs testall

install: bs
	$(INS) -s bs $(INSDIR)

clean:
	-rm -f *.o bs
