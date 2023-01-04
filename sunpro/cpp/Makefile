#
#	@(#)Makefile 1.1 87/01/08 SMI; from S5R2 1.9
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

CPP	= /lib/cpp
VROOT	= ../vroot
VROOTLIB = $(VROOT)/vroot.a
CPPFLAGS = -DSUNPRO -DFLEXNAMES -I$(VROOT)
COPTS	= -O
CFLAGS	= $(COPTS) $(CPPFLAGS)
UNDEF   = -Qoption cpp -undef
SUNDEFS	= -Dunix -Dsun
SRCS	= cpy.c cpp.c
HDRS	= $(VROOT)/vroot.h $(VROOT)/report.h

.DEFAULT:
	sccs get -G$@ $@

cpp :	cpp.o cpy.o rodata.o $(VROOTLIB)
	$(CC) $(LDFLAGS) -o cpp cpp.o rodata.o cpy.o  $(VROOTLIB)

cpp.o cpy.o: $(HDRS)

cpy.c : cpy.y yyfix.sh
	$(YACC) cpy.y
	/bin/sh ./yyfix.sh yyexca yyact yypact yypgo yyr1 yyr2 yychk yydef
	mv y.tab.c cpy.c

cpy.o :	cpy.c yylex.c
	$(CC) $(CFLAGS) -c cpy.c

cpp.o : cpp.c
	@set -x; if mc68010; then \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) -Dmc68000 -Dmc68010 -c cpp.c;\
	elif mc68020; then \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) -Dmc68000 -Dmc68020 -c cpp.c;\
	elif sunrise; then \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) -Dsunrise -c cpp.c;\
	else	echo 'Unknown machine???'; exit 2;\
	fi

rodata.o : cpy.c
	$(CC) $(CFLAGS) -R -c rodata.c

$(HDRS) $(VROOTLIB): FRC
	cd $(@D); $(MAKE) $(@F)

FRC:

clean :
	rm -f *.s *.o
	rm -f cpy.c rodata.c cpp

install : cpp
	$(INS) -s cpp $(DESTDIR)/usr/sunpro

depend: $(SRCS) $(HDRS) yylex.c
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CPP) -M $(CPPFLAGS) $$i >> makedep ); \
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

cpy.o: cpy.c
cpy.o: ./yylex.c
cpp.o: cpp.c
cpp.o: /usr/include/stdio.h
cpp.o: /usr/include/ctype.h
cpp.o: /usr/include/sys/file.h
cpp.o: /usr/include/sys/fcntl.h
cpp.o: ../vroot/vroot.h
cpp.o: /usr/include/stdio.h
cpp.o: ../vroot/report.h
cpp.o: /usr/include/stdio.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
