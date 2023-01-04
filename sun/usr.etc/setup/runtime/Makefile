#
# @(#)Makefile 1.1 86/09/25 SMI
#
#	Build setup runtime support library

CFLAGS = -O

OBJS=initialize.o attributes.o workstation.o error_checker.o runtime_error.o \
	check_legal.o read_config.o hardware.o disk.o hard_part.o controller.o \
	client.o soft_part.o callback.o setup_message.o util.o \
	software.o upgrade.o install.o arch_served.o float_disk.o \
	install_server.o label.o tape.o card.o card_parser.o \
	upgrade_disk.o upgrade_oswg.o
SRC=initialize.c attributes.c workstation.c error_checker.c runtime_error.c \
	check_legal.c read_config.c hardware.c disk.c hard_part.c controller.c \
	client.c soft_part.c callback.c setup_message.c util.c \
	software.c upgrade.c install.c arch_served.c float_disk.c \
	install_server.c label.c tape.c card.c card_parser.c\
	upgrade_disk.c upgrade_oswg.c
SCRIPTS = xtr_root xtr_rootarch xtr_usrarch xtr_symlinks \
	xtr_standalone xtr_standpub xtr_client copy_client fix_client \
	fix_hostname fix_domainname fix_rc.boot fix_servers

MISCFILES = setup.cards setuphardware.file

all: libsetup.a setup.config setup.cards

derf: derf.o libsetup.a
	cc -g -o derf derf.o libsetup.a -lsunwindow

libsetup.a: $(OBJS)
	ar ru libsetup.a $?
	ranlib libsetup.a

setup.config: SCCS/s.setup.config
	sccs get setup.config

setup.cards: SCCS/s.setup.cards
	sccs get setup.cards

clean:
	rm -f *.o libsetup.a *.BAK 

install: libsetup.a ${MISCFILES} ${SCRIPTS}
	-mkdir ${DESTDIR}/usr/etc/setup.files && \
		chown bin ${DESTDIR}/usr/etc/setup.files && \
		chmod 755 ${DESTDIR}/usr/etc/setup.files
	for i in ${MISCFILES} ${SCRIPTS}; do \
		(install -c $$i ${DESTDIR}/usr/etc/setup.files/$$i); done

depend:
	grep '^#include' ${SRC} | \
		sed 's/:[^<]*<\([^>]*\)>.*/: \/usr\/include\/\1/' | \
		sed 's/:[^"]*"\([^"]*\)".*/: \1/' | \
		sed 's/\.c/.o/' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
			else rec = rec " " $$2 } } \
		END { print rec } ' > makedep
	echo '$$r makedep' >>eddep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

initialize.o: setup_runtime.h /usr/include/nlist.h
attributes.o: setup_runtime.h
workstation.o: setup_runtime.h
error_checker.o: setup_runtime.h install.h
runtime_error.o: /usr/include/stdio.h
check_legal.o: setup_runtime.h /usr/include/ctype.h /usr/include/netinet/in.h
check_legal.o: /usr/include/sys/socket.h /usr/include/arpa/inet.h
read_config.o: /usr/include/ctype.h /usr/include/strings.h setup_runtime.h
hardware.o: setup_runtime.h /usr/include/sys/socket.h /usr/include/net/if.h
hardware.o: /usr/include/sys/ioctl.h /usr/include/sun/dklabel.h
hardware.o: /usr/include/sun/dkio.h /usr/include/sys/stat.h
disk.o: setup_runtime.h
hard_part.o: setup_runtime.h
controller.o: setup_runtime.h
client.o: setup_runtime.h
soft_part.o: setup_runtime.h
callback.o: setup_runtime.h
setup_message.o: setup_runtime.h
util.o: setup_runtime.h /usr/include/ctype.h
software.o: setup_runtime.h
upgrade.o: /usr/include/stdio.h /usr/include/ctype.h /usr/include/netdb.h
upgrade.o: setup_runtime.h install.h
install.o: /usr/include/stdio.h /usr/include/ctype.h /usr/include/sys/ioctl.h
install.o: /usr/include/setjmp.h setup_runtime.h install.h
install.o: /usr/include/sys/stat.h /usr/include/sys/wait.h
arch_served.o: setup_runtime.h
float_disk.o: setup_runtime.h
install_server.o: /usr/include/ctype.h setup_runtime.h install.h
install_server.o: /usr/include/sys/types.h /usr/include/sys/time.h
install_server.o: /usr/include/sys/socket.h /usr/include/sys/ioctl.h
install_server.o: /usr/include/net/if.h /usr/include/netinet/in.h
install_server.o: /usr/include/sys/mbuf.h /usr/include/netinet/if_ether.h
install_server.o: /usr/include/net/nit.h
label.o: setup_runtime.h /usr/include/sys/ioctl.h /usr/include/sun/dkio.h
label.o: /usr/include/sun/dklabel.h
tape.o: /usr/include/stdio.h /usr/include/signal.h setup_runtime.h install.h
card.o: setup_runtime.h
card_parser.o: /usr/include/ctype.h /usr/include/strings.h setup_runtime.h
upgrade_disk.o: setup_runtime.h install.h
upgrade_oswg.o: setup_runtime.h install.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
