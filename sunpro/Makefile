#
#	@(#)Makefile 1.7 87/04/17 SMI
#

# The subdirs we care about with ".x" suffixes added
SUBDIRS = compile.x cpp.x ld.x m4.x make.x vroot.x filemerge.x

all:	$(SUBDIRS:.x=-all)
$(SUBDIRS:.x=-all):
	cd $(@:-all=); $(MAKE)

install:	local-install $(SUBDIRS:.x=-install)
$(SUBDIRS:.x=-install):
	cd $(@:-install=); $(MAKE) DESTDIR=$(DESTDIR) install
local-install: install_sunpro
	cp install_sunpro $(DESTDIR)/usr/sunpro
	chmod 555 $(DESTDIR)/usr/sunpro/install_sunpro
	-mkdir $(DESTDIR)/usr/sunpro
install_sunpro:
	if [ "sun2" = "`arch`" ] ; \
	then sccs get install_sunpro.sun2 -Ginstall_sunpro ; \
	else sccs get install_sunpro.sun3 -Ginstall_sunpro ; \
	fi
	if [ "sun2" = "`arch`" ] ; \
	then sccs get install_sunpro.sun2 -Ginstall_sunpro ; \
	else sccs get install_sunpro.sun3 -Ginstall_sunpro ; \
	fi

clean:	$(SUBDIRS:.x=-clean)
	rm -f install_sunpro

$(SUBDIRS:.x=-clean):
	cd $(@:-clean=); $(MAKE) clean
