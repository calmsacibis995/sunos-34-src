#
# @(#)Makefile 1.1 86/07/07 Copyright 1985 Sun Micro; from UCB 4.2 10/22/82
#
DESTDIR=
CFLAGS=	-g
OBJS= indent.o io.o lexi.o parse.o pr_comment.o args.o
SRCS= indent.c io.c lexi.c parse.c pr_comment.c args.c

indent: ${OBJS}
	cc ${CFLAGS} -o indent ${OBJS}

objsall: ${OBJS}

clean:
	rm -f ${OBJS} indent

indent.o: indent.c
io.o: io.c
lexi.o: lexi.c
parse.o: parse.c
pr_comment.o: pr_comment.c
$(OBJS): indent_globs.h indent_codes.h

.DEFAULT:
	sccs get $<

install: indent
	install -s indent ${DESTDIR}/usr/bin/indent
depend:
	cc -M ${SRCS} | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >> eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

indent.o: indent.c ./indent_globs.h /usr/include/stdio.h ./indent_codes.h
io.o: io.c ./indent_globs.h /usr/include/stdio.h
lexi.o: lexi.c ./indent_globs.h /usr/include/stdio.h ./indent_codes.h
lexi.o: /usr/include/ctype.h
parse.o: parse.c ././indent_globs.h /usr/include/stdio.h ././indent_codes.h
pr_comment.o: pr_comment.c ./indent_globs.h /usr/include/stdio.h
args.o: args.c ./indent_globs.h /usr/include/stdio.h /usr/include/sys/types.h
args.o: /usr/include/ctype.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
