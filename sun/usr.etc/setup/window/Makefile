#
# @(#)Makefile @(#)Makefile 1.1 86/09/25 SMI
#

OBJS = setup.o image.o ws_panel.o generic.o disk_panel.o client_panel.o \
       button.o message.o software_panel.o defaults_panel.o confirm.o \
       suntools.o suntools_menu.o toolmerge.o selection_svc.o

CFILES = setup.c image.c ws_panel.c generic.c disk_panel.c client_panel.c \
         button.c message.c software_panel.c defaults_panel.c confirm.c \
	 suntools.c suntools_menu.c toolmerge.c selection_svc.c

AWK = awk

CFLAGS = -O -I../runtime
LIBS =	../runtime/libsetup.a -lsuntool -lsunwindow -lpixrect

setup.window: ${OBJS} ../runtime/libsetup.a
	${CC} ${CFLAGS} ${LDFLAGS} -o setup.window ${OBJS} ${LIBS}

#
# use the source of suntools.c and selection_svc.c from the window system
# source directory by having rules that explicitly depend on their SCCS files
#

WINFILES=/usr/src/sun/usr.bin/suntool
#WINFILES=.

suntools.c: ${WINFILES}/SCCS/s.suntools.c
	sccs get ${WINFILES}/suntools.c

suntools_menu.c: ${WINFILES}/SCCS/s.suntools_menu.c
	sccs get ${WINFILES}/suntools_menu.c

selection_svc.c: ${WINFILES}/SCCS/s.selection_svc.c
	sccs get ${WINFILES}/selection_svc.c

clean:
	rm -f *.o core setup.window

lint:
	lint $(CFLAGS) $(CFILES) /usr/lib/lint/llib_lpixrect.ln \
		/usr/lib/lint/llib_lsunwindow.ln \
		/usr/lib/lint/llib_lsuntool.ln

install: setup.window
	install -s setup.window ${DESTDIR}/usr/etc/setup.files/setup.window

depend:
	grep '^#include' ${CFILES} | \
     	      sed 's/:[^<]*<\([^>]*\)>.*/: \/usr\/include\/\1/' | \
	      sed 's/:[^"]*"\([^"]*\)".*/: \1/' | \
	      sed 's/\.c/.o/' | \
	${AWK} ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '$$r makedep' >>eddep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	rm -f Makefile.bak
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

setup.o: /usr/include/stdio.h setup.h images/setup.icon
image.o: setup.h images/wcdisk.icon images/controller1.icon
image.o: images/controller2.icon images/xy_disk.icon images/xy_controller1.icon
image.o: images/xy_controller2.icon images/sd_disk.icon
image.o: images/sd_controller1.icon images/sd_controller2.icon
image.o: images/partition.pr images/client.icon images/card.icon
image.o: images/cycle.pr
ws_panel.o: setup.h
generic.o: setup.h
disk_panel.o: setup.h
client_panel.o: setup.h
button.o: setup.h /usr/include/signal.h
message.o: setup.h /usr/include/suntool/textsw.h
software_panel.o: setup.h
defaults_panel.o: setup.h
confirm.o: /usr/include/sys/file.h setup.h
suntools.o: /usr/include/suntool/tool_hs.h /usr/include/sys/ioctl.h
suntools.o: /usr/include/sys/dir.h /usr/include/sys/file.h
suntools.o: /usr/include/sys/wait.h /usr/include/sys/resource.h
suntools.o: /usr/include/sys/stat.h /usr/include/errno.h /usr/include/stdio.h
suntools.o: /usr/include/pwd.h /usr/include/ctype.h
suntools.o: /usr/include/sunwindow/defaults.h /usr/include/suntool/menu.h
suntools.o: /usr/include/suntool/wmgr.h /usr/include/suntool/selection.h
suntools.o: /usr/include/suntool/selection_svc.h
suntools.o: /usr/include/suntool/walkmenu.h
suntools_menu.o: /usr/include/suntool/tool_hs.h /usr/include/stdio.h
suntools_menu.o: /usr/include/sys/stat.h /usr/include/ctype.h
suntools_menu.o: /usr/include/suntool/wmgr.h /usr/include/suntool/icon_load.h
suntools_menu.o: /usr/include/suntool/walkmenu.h
selection_svc.o: /usr/include/sys/types.h /usr/include/sunwindow/notify.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
