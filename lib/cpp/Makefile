#
#	@(#)Makefile 1.1 86/09/24 SMI; from S5R2 1.9
#

INS	= install
INSDIR	= $(DESTDIR)/lib
#
# Note: if the installed cpp doesn't know about -undef, do
#	rm cpp.o; make UNDEF="" ; make install
#	(/lib/cpp now knows about -undef, but may have bogus predefinitions)
#	rm cpp.o; make install
#	(/lib/cpp now knows about -undef, but predefines only the names below)
#
CFLAGS	= -O -DFLEXNAMES
UNDEF   = -Qoption cpp -undef
SUNDEFS	= -Dunix -Dsun

cpp :	cpp.o cpy.o rodata.o
	$(CC) $(LDFLAGS) -o cpp cpp.o rodata.o cpy.o  

cpy.c : cpy.y
	$(YACC) cpy.y
	/bin/sh ./yyfix.sh yyexca yyact yypact yypgo yyr1 yyr2 yychk yydef
	mv y.tab.c cpy.c

cpy.o :	cpy.c yylex.c
	$(CC) $(CFLAGS) -c cpy.c

cpp.o : cpp.c
	if mc68010; then \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) -Dmc68000 -Dmc68010 -c cpp.c;\
	elif mc68020; then \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) -Dmc68000 -Dmc68020 -c cpp.c;\
	else	echo 'Unknown machine???'; exit 2;\
	fi

rodata.o : cpy.c
	$(CC) $(CFLAGS) -R -c rodata.c

clean :
	rm -f *.s *.o
	rm -f cpy.c rodata.c cpp

install : cpp
	$(INS) -s cpp $(INSDIR)
