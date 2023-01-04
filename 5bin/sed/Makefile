#
# @(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.7
#
#	sed make file

IFLAG =
B10 =
CFLAGS = -O $(B10)
LDFLAGS = $(IFLAG)
FILES = sed0.o sed1.o

sed:	$(FILES)
	$(CC) $(LDFLAGS) -o sed $(FILES)

$(FILES): sed.h

install: sed
	install -s sed $(DESTDIR)/usr/5bin

clean:
	rm -f $(FILES) sed
