#
# @(#)makefile 1.3 87/03/16 SMI; from S5R2 1.4
#
#	Curses Library Low Level Makefile
#
#	CFLAGS has -DVIDEO & -DKEYPAD for the libcurses.a library.  They could
#		be put into a separate variable if desired, but I saw no reason
#		for doing so.
#	DFLAGS & TFLAGS are for debugging & tracing respectively.
#		They all contain the DEBUG flag because it is
#		thought that any of the styles of compiling is still debugging.
#		Also the DEBUG flag will create very large files and it is not
#		recommended that all of the files be compiled with the DEBUG
#		flag defined.  This will make 'ar' die because the library will
#		exceed 2065 blocks in size.  But I see no reason not to compile
#		individual or groups of files with these flags defined.  These
#		flags can't be used on a pdp11/70 or other small machines
#		because of the huge sizes of .o files produced for the library.
#	FILES is used for the naming of the object files.
#	O is used to distinguish between compiling, debugging, & tracing.
#	MINICURSES is a flag used to compile a small version of libcurses.
#		It should not be used for the compiling of libcurses.a.  It is
#		strictly for the application programmers convenience.
#		If MINICURSES is desired with ti4 or show then type:
#			"make <[ti4][show] MINICURSES=-DMINICURSES"
#
INS		= install
LIB		= $(DESTDIR)/usr/5lib
CURSES		= libcurses.a
BIN		= $(DESTDIR)/usr/5bin
INC		= $(DESTDIR)/usr/5include
O		= o
MINICURSES	=
CFLAGS		= -DVIDEO -DKEYPAD -O -DS5EMUL
DFLAGS		= -DVIDEO -DKEYPAD -DDEBUG -I. -DS5EMUL
TFLAGS		= -DVIDEO -DKEYPAD -DDEBUG -O -I. -DS5EMUL
FILES		= __cflush.$(O) __sscans.$(O) _blanks.$(O) _c_clean.$(O) \
		_clearhl.$(O) _clearline.$(O) _comphash.$(O) _delay.$(O) \
		_delchars.$(O) _dellines.$(O) _dumpwin.$(O) _ec_quit.$(O) \
		_fixdelay.$(O) _forcehl.$(O) _hlmode.$(O) _id_char.$(O) \
		_init_cost.$(O) _inschars.$(O) _insmode.$(O) _kpmode.$(O) \
		_line_free.$(O) _ll_move.$(O) _outch.$(O) _outchar.$(O) \
		_pos.$(O) _reset.$(O) _scrdown.$(O) _scrollf.$(O) _sethl.$(O) \
		_setmode.$(O) _setwind.$(O) _shove.$(O) _sprintw.$(O) \
		_sputc.$(O) _syncmodes.$(O) _tscroll.$(O) _window.$(O) \
		addch.$(O) addstr.$(O) baudrate.$(O) beep.$(O) box.$(O) \
		capnames.$(O) cbreak.$(O) chktypeahd.$(O) clear.$(O) \
		clearok.$(O) clreolinln.$(O) clrtobot.$(O) clrtoeol.$(O) \
		cntcostfn.$(O) crmode.$(O) curses.$(O) def_prog.$(O) \
		def_shell.$(O) delayoutpt.$(O) delch.$(O) deleteln.$(O) \
		delwin.$(O) doprnt.$(O) doscan.$(O) doupdate.$(O) draino.$(O) \
		echo.$(O) endwin.$(O) erase.$(O) erasechar.$(O) fixterm.$(O) \
		flash.$(O) flushinp.$(O) getch.$(O) getstr.$(O) idlok.$(O) \
		gettmode.$(O) has_ic.$(O) has_il.$(O) idln.getst.$(O) \
		initkeypad.$(O) initscr.$(O) insch.$(O) insertln.$(O) \
		intrflush.$(O) keypad.$(O) killchar.$(O) leaveok.$(O) \
		line_alloc.$(O) ll_refresh.$(O) longname.$(O) m_addch.$(O) \
		m_addstr.$(O) m_clear.$(O) m_erase.$(O) m_move.$(O) meta.$(O) \
		m_refresh.$(O) m_tstp.$(O) makenew.$(O) miniinit.$(O) \
		move.$(O) mvcur.$(O) mvprintw.$(O) mvscanw.$(O) mvwin.$(O) \
		mvwprintw.$(O) mvwscanw.$(O) naps.$(O) newpad.$(O) \
		newterm.$(O) newwin.$(O) nl.$(O) nocbreak.$(O) nocrmode.$(O) \
		nodelay.$(O) noecho.$(O) nonl.$(O) noraw.$(O) nttychktrm.$(O) \
		overlay.$(O) overwrite.$(O) pnoutrfrsh.$(O) prefresh.$(O) \
		printw.$(O) putp.$(O) raw.$(O) reset_prog.$(O) resetshell.$(O) \
		resetterm.$(O) resetty.$(O) restarttrm.$(O) saveterm.$(O) \
		savetty.$(O) scanw.$(O) scroll.$(O) scrollok.$(O) \
		set_term.$(O) setbuffred.$(O) setterm.$(O) setupterm.$(O) \
		showstring.$(O) subwin.$(O) tgetent.$(O) tgetflag.$(O) \
		tgetnum.$(O) tgetstr.$(O) tgoto.$(O) touchwin.$(O) tparm.$(O) \
		tputs.$(O) traceonoff.$(O) tstp.$(O) two.twostr.$(O) \
		typeahead.$(O) unctrl.$(O) vidattr.$(O) vidputs.$(O) \
		vsscanf.$(O) wattroff.$(O) wattron.$(O) \
		wattrset.$(O) wnoutrfrsh.$(O) wprintw.$(O) wrefresh.$(O) \
		writechars.$(O) wscanw.$(O) wstandend.$(O) wstandout.$(O)

.SUFFIXES:	.o .d .t

.c.o:	
			$(PROF)$(CC) -p $(CFLAGS) $(MINICURSES) -c $*.c
			$(PROF)$(LD) -x -r $*.o
			$(PROF)mv a.out profiled/$*.o
			$(CC) $(CFLAGS) $(MINICURSES) -c $*.c
			$(LD) -x -r $*.o
			mv a.out $*.o

.c.d:	
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(DFLAGS) $(MINICURSES) -c $*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o
.c.t:	
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(TFLAGS) $(MINICURSES) -c $*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o

all:		$(CURSES)

$(CURSES):	$(FILES) curses.h curshdr.h term.h unctrl.h curses.ext curses.c
		$(PROF)cd profiled; ar rv ../libcurses_p.a `lorder *.$(O) | tsort`
		ar rv $(CURSES) `lorder *.$(O) | tsort`

debug:
		$(MAKE) O=d CURSES=libdcurses.a PROF="@#"

trace:
		$(MAKE) O=t CURSES=libtcurses.a PROF="@#"

show:		libcurses.a show.o
		$(CC) $(LDFLAGS) $(MINICURSES) show.o libcurses.a -o show
		rm -f show.o

term.h capnames.c: caps maketerm.ex
		rm -f term.h capnames.c
		ex - < maketerm.ex
		rm -f boolnames boolvals bool numnames numvals num strnames strvals str term.h.new
		chmod -w term.h capnames.c

termcap.c:	termcap.form termcap.ed caps
		rm -f termcap.c
		ed - < termcap.ed
		rm -f bool num str
		chmod -w termcap.c

tic:		tic.c capnames.o
		-if pdp11; \
		then \
			$(CC) -i $(LDFLAGS) $(CFLAGS) $(MINICURSES) tic.c capnames.o -o tic; \
			rm -f tic.o; \
		else \
			$(CC) $(LDFLAGS) $(CFLAGS) $(MINICURSES) tic.c capnames.o -o tic; \
			rm -f tic.o; \
		fi

ti4:		libcurses.a ti4.o
		$(CC) $(LDFLAGS) $(MINICURSES) ti4$(O) libcurses.a -o ti4
		rm -f ti4.o

ti4.o:		
		$(CC) -O mkti4.c -o mkti4
		mkti4 >ti4.c
		$(CC) -c $(CFLAGS) $(MINICURSES) ti4.c

clean:	
		rm -f *.o profiled/*.o

clobber:	clean
		rm -f libcurses.a libcurses_p.a tic show ti4

install:	libcurses.a tic curses.h term.h unctrl.h
		$(PROF)$(INS) -m 644 libcurses_p.a $(LIB)
		$(INS) -m 644 libcurses.a $(LIB)
		$(INS) -s tic $(BIN)
		$(INS) -m 644 curses.h $(INC)
		$(INS) -m 644 term.h $(INC)
		$(INS) -m 644 unctrl.h $(INC)
		$(PROF)ranlib $(LIB)/libcurses_p.a
		ranlib $(LIB)/libcurses.a

install_h: curses.h term.h unctrl.h
		$(INS) -m 644 curses.h $(INC)
		$(INS) -m 644 term.h $(INC)
		$(INS) -m 644 unctrl.h $(INC)
