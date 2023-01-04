#
# @(#)Makefile 1.1 86/09/25 SMI
#

CFLAGS = -O -I../runtime

LIBS  =  ../runtime/libsetup.a  -lcurses -ltermcap -lsunwindow
TTY_LIB =  tty_init.o tty_main.o  tty_workstation.o \
  tty_glue.o tty_error.o   tty_edit_disk.o tty_navigate.o \
  tty_client.o tty_card.o tty_list.o tty_software.o \
  tty_setup_execute.o tty_defaults.o
    
FORM_LIB = form.o form_item.o form_attr.o form_button.o form_text.o \
  form_input.o form_choice.o form_toggle.o form_match.o

SRC = tty_init.c tty_main.c  tty_workstation.c \
  tty_glue.c tty_error.c   tty_edit_disk.c tty_navigate.c \
  tty_client.c tty_card.c tty_list.c tty_software.c \
  tty_setup_execute.c  tty_defaults.c\
  form.c form_item.c form_attr.c form_button.c form_text.c \
  form_input.c form_choice.c form_toggle.c form_match.c
  

setup.tty: $(TTY_LIB) form.a ../runtime/libsetup.a Makefile
	cc $(CFLAGS) $(TTY_LIB) form.a $(LIBS) -o setup.tty
	

form.a:	$(FORM_LIB)
	ar ruv form.a $(FORM_LIB)
	ranlib form.a	


clean:
	rm -f *.o core setup.tty *.BAK


install: setup.tty
	install -s setup.tty ${DESTDIR}/usr/etc/setup.files/setup.tty


depend:
	grep '^#include' ${SRC} | \
     	      sed 's/:[^<]*<\([^>]*\)>.*/: \/usr\/include\/\1/' | \
	      sed 's/:[^"]*"\([^"]*\)".*/: \1/' | \
	      sed 's/\.c/.o/' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
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

tty_init.o: tty_global.h user_info.h form_match.h
tty_main.o: tty_global.h tty_list.h /usr/include/signal.h
tty_workstation.o: tty_global.h tty_item.h
tty_glue.o: tty_global.h
tty_error.o: tty_global.h tty_item.h
tty_edit_disk.o: tty_global.h tty_item.h
tty_navigate.o: tty_global.h tty_item.h
tty_client.o: tty_global.h tty_item.h tty_list.h
tty_card.o: tty_global.h tty_item.h tty_list.h
tty_list.o: tty_global.h tty_item.h tty_list.h
tty_software.o: tty_global.h tty_item.h tty_list.h
tty_setup_execute.o: tty_global.h tty_item.h
tty_defaults.o: tty_global.h tty_item.h
form.o: form.h form_attr.h
form_item.o: form.h object.h form_attr.h form_match.h
form_attr.o: form.h object.h form_attr.h
form_button.o: form.h object.h form_attr.h form_match.h
form_text.o: form.h object.h form_attr.h form_match.h user_info.h
form_text.o: /usr/include/ctype.h
form_input.o: tty_global.h user_info.h form_match.h
form_choice.o: form.h object.h form_attr.h form_match.h user_info.h
form_choice.o: /usr/include/ctype.h
form_toggle.o: form.h object.h form_attr.h form_match.h user_info.h
form_toggle.o: /usr/include/ctype.h
form_match.o: tty_global.h user_info.h form_match.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
