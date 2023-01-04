#
# @(#)Makefile 1.1 86/09/24 SMI; from UCB 4.2 83/07/08
#
CFLAGS=-O
YFLAGS=-d
FILES=awk.lx.o b.o main.o token.o tran.o lib.o run.o parse.o proctab.o freeze.o
SOURCE=awk.def awk.g.y awk.lx.l b.c lib.c main.c parse.c \
	makeprctab.c freeze.c\
	run.c token.c tran.c

awk:	$(FILES) awk.g.o
	$(CC) $(LDFLAGS) awk.g.o  $(FILES)  -lm -o awk

y.tab.h:	awk.g.o

awk.h:	y.tab.h
	-cmp -s y.tab.h awk.h || cp y.tab.h awk.h

$(FILES):	awk.h awk.def

token.o:	awk.h tokendefs

tokendefs:	y.tab.h
	rm -f tokendefs
	ed - <tokenscript

get:
	for i in $(SOURCE) Makefile tokenscript README; do \
		cp /usr/src/bin/awk/$$i .;
	done

install:	awk
	install -s awk ${DESTDIR}/bin

profile:	awk.g.o $(FILES) mon.o
	$(CC) -p awk.g.o $(FILES) mon.o -lm

find:
	egrep -n "$(PAT)" *.[ylhc] awk.def

list:
	-pr $(SOURCE) Makefile tokenscript README EXPLAIN

lint:
	lint -spu b.c main.c token.c tran.c run.c lib.c parse.c -lm |\
		egrep -v '^(error|free|malloc)'

proctab.c:	./makeprctab
	./makeprctab >proctab.c

./makeprctab:	awk.h makeprctab.o token.o
	$(CC) -o ./makeprctab makeprctab.o token.o

clean:
	-rm -f *.o t.* *temp* *.out *junk* y.tab.* awk.h ./makeprctab proctab.c awk yacc* tokendefs awk.lx.c awk.g.c core errs

diffs:
	-for i in $(SOURCE); do echo $$i:; diff $$i /usr/src/cmd/awk | ind; done
