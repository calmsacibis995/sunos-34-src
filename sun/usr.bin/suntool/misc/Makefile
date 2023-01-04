#
# @(#)Makefile 1.3 87/01/07 SMI
#

CFILESALL=	align_equals.c capitalize.c clear_functions.c get_selection.c \
		insert_brackets.c setkeys.c shift_lines.c
OBJFILES=	align_equals.o capitalize.o clear_functions.o get_selection.o \
		insert_brackets.o setkeys.o shift_lines.o
GPROFFILESALL= ${CFILESALL:.c=.pg}
HDRSALL= 
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

INCLUDE=/usr/include
AWK=	/bin/awk
TOUCH=	touch -cf
CHOWN=	/etc/chown -f
DESTDIR=
CPUFLAG=
PROFILE=

DBX=
CFLAGS=	-O -I$(INCLUDE)
CFLAGS_LOCAL= 

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

LDFLAGS= ${DBX}
CTAGS=	/usr/ucb/ctags -w
LIBS=	-lsuntool -lsunwindow -lpixrect
TERMLIB= -ltermcap
LIBFILE= libmisc.a
BASETOOLS_BIN= suntools
OTHERTOOLS_BIN= othertools
GPROF_MERGE_LIB= libtoolmerge_p.a
.SUFFIXES: .pg


BASE_BINS=	align_equals capitalize clear_functions get_selection \
	insert_brackets shift_lines 	   
OTHERS_BINS= setkeys

# CMDS are all of the programs made in this directory
CMDS=	${BINS}
#		test_service

#Note: not yet converted to -A-R
#.c.o:
#	cc $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) -A-R -c $<
.c.o:
	cc ${CFLAGS_NOT_ROI_DATA} -c $<

.c.pg:
	if [ -f $*.o ]; then \
		mv $*.o $*.o.tmp; \
	fi
	cc ${CFLAGS_NOT_ROI_DATA} -pg -c $<
	-mv $*.o $*.pg
	if [ -f $*.o.tmp ]; then \
		mv $*.o.tmp $*.o; \
	fi

${LIBFILE}: ${OBJFILES}
	ar uv ${LIBFILE} $?
	ranlib ${LIBFILE}

#	-for i in ${SUBDIRS}; do \
#	(	echo "Descending into directory: $$i"; \
#		cd $$i; \
#		make ${MFLAGS} "LDFLAGS=${LDFLAGS}" \
#		     "CFLAGS=${CFLAGS}" "DBX=${DBX}" "LIBS=${LIBS}" \
#	); done

objsall: ${OBJFILES}
#	-for i in ${SUBDIRS}; do \
#	(	echo "Descending into directory: $$i"; \
#		cd $$i; \
#		make ${MFLAGS} "LDFLAGS=${LDFLAGS}" \
#		     "CFLAGS=${CFLAGS}" "DBX=${DBX}" "LIBS=${LIBS} objsall" \
#	); done

gprof:: ${GPROFFILESALL}
	ar uvl ../${GPROF_MERGE_LIB} ${GPROFFILESALL}

standalone:	${BINS}

insert_brackets: insert_brackets.c $(LIBS)    
	cc -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o insert_brackets insert_brackets.c \
	$(LIBS)  

align_equals: align_equals.c $(LIBS)
	cc -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o align_equals align_equals.c $(LIBS)

capitalize: capitalize.c    
	cc -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o capitalize capitalize.c   

shift_lines: shift_lines.c $(LIBS)  
	cc -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o shift_lines shift_lines.c $(LIBS)  

setkeys:	setkeys.c keytables.o.link
	cc -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} ${LDFLAGS} -o setkeys setkeys.c \
		keytables.o.link ${TERMLIB}

test_service:	test_service.o $(LIBS)
	cc -g -o test_service test_service.o $(LIBS)

test_service.o:	selection_svc.h selection_impl.h selection_attributes.h	\
		test_service.c
	cc -c -g -DTESTING -I. -I/usr/include test_service.c

clear_functions: clear_functions.c $(LIBS)  
	cc -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o clear_functions clear_functions.c $(LIBS) 

get_selection: get_selection.c $(LIBS)   
	cc -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o get_selection get_selection.c $(LIBS) 

FRC:
# non-existent so always younger

install: install_bins 
#	-for i in ${SUBDIRS}; do \
#		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	
install_bins:
	-for i in ${BASE_BINS}; do \
		rm -f ${DESTDIR}/usr/bin/$$i; \
		ln -s ${BASETOOLS_BIN} ${DESTDIR}/usr/bin/$$i; \
	done
	-for i in ${OTHERS_BINS}; do \
		rm -f ${DESTDIR}/usr/bin/$$i; \
		ln -s ${OTHERTOOLS_BIN} ${DESTDIR}/usr/bin/$$i; \
	done

clean:
	rm -f *.o core $(CMDS) *.pg
#	for i in ${SUBDIRS}; do (cd $$i; make ${MFLAGS} clean); done

cleanCMDS:
	rm -f $(CMDS) 

$(SRCSALL):
	sccs get $@;

srcsget: $(SRCSALL)

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)

lint:
	lint $(CFLAGS) $(CFILESALL) /usr/lib/lint/llib_lpixrect.ln \
		/usr/lib/lint/llib_lsunwindow.ln \
		/usr/lib/lint/llib_lsuntool.ln

depend:
	make_depend -I${INCLUDE} ${CFILESALL}

#include Make.depend

# DO NOT DELETE THIS LINE -- make depend uses it
align_equals.o: align_equals.c \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/string_utils.h

capitalize.o: capitalize.c \
	${INCLUDE}/sunwindow/sun.h

clear_functions.o: clear_functions.c \
	${INCLUDE}/sunwindow/notify.h \
	${INCLUDE}/suntool/selection_svc.h

get_selection.o: get_selection.c \
	${INCLUDE}/suntool/selection_svc.h \
	${INCLUDE}/suntool/selection_attributes.h \
	${INCLUDE}/sunwindow/attr.h

insert_brackets.o: insert_brackets.c \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/string_utils.h

setkeys.o: setkeys.c \
	${INCLUDE}/sundev/kbd.h \
	${INCLUDE}/sundev/kbio.h

shift_lines.o: shift_lines.c \
	${INCLUDE}/sunwindow/sun.h \
	${INCLUDE}/sunwindow/string_utils.h



# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# SEE HOW make depend WORKS FOR FURTHER INFO
