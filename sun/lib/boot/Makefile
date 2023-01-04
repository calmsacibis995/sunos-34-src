#
# @(#)Makefile	1.3 (Sun) 1/9/87
# compiler fixed-point test
#
compile = /usr/src/lib/compile
cpp = /usr/src/lib/cpp
libc = /usr/src/lib/libc
libm = /usr/src/usr.lib/libm
pcc = /usr/src/sun/lib/pcc
lint = /usr/src/usr.bin/lint
c2 = /usr/src/sun/lib/c2
as = /usr/src/sun/bin/as
ld = /usr/src/bin/ld
f77pass1 = /usr/src/usr.lib/f77pass1
iropt = /usr/src/sun/usr.lib/iropt
alt = /usr/tmp/new
newcc = cc -Qpath $(alt) -L$(alt)

CMDS = cc f77 pc
INSTALLCMDS = /bin/cc /usr/bin/f77 /usr/ucb/pc /usr/bin/lint

#
# step 1: make new components using existing components.
#
all: sanity
	makeinstall

step1cc: clean
	cd $(compile) ; make
	touch $@

step1cpp: step1cc
	cd $(cpp) ; make
	touch $@

step1pcc: step1cpp
	cd $(pcc) ; make
	touch $@

step1lint: step1pcc
	cd $(lint) ; make
	touch $@

step1c2: step1lint
	cd $(c2) ; make
	touch $@

step1as: step1c2
	cd $(as) ; make
	touch $@

step1ld: step1as
	cd $(ld) ; make
	touch $@

step1iropt: step1ld
	cd $(iropt) ; make
	touch $@

step1f77pass1: step1iropt
	cd $(f77pass1) ; make
	touch $@

#
# Install the new compiler passes.
#
step1:	step1f77pass1
	cd $(compile) ; install compile $(alt)/compile ; \
	    for i in ${CMDS}; do \
		(rm -f $(alt)/$$i; ln -s compile $(alt)/$$i)\
	    done
	cd $(cpp) ; install cpp $(alt)/cpp
	cd $(pcc) ; install comp $(alt)/ccom
	cd $(pcc) ; install cg $(alt)/cg
	cd $(pcc) ; install fort $(alt)/pc1
	cd $(c2) ; install c2 $(alt)/c2
	cd $(as) ; install as $(alt)/as
	cd $(ld) ; install ld $(alt)/ld
	cd $(iropt) ; install iropt $(alt)/iropt
	cd $(f77pass1) ; install f77pass1 $(alt)/f77pass1
	touch $@

#
# Initialize for step 2
#
step2clean: step1
	cd $(libc) ; make clean > /dev/null 2>&1 ; rm -f errs
	cd $(libm) ; make clean > /dev/null 2>&1 ; rm -f errs
	touch $@

#
# step 2: Make a library with the new components and install it. 
#
step2libc: step2clean
	cd $(libc) ; make CC="$(alt)/cc -Qpath $(alt)" AS="$(alt)/as" \
	    >> errs 2>&1
	touch $@

step2libm: step2libc
	cd $(libm) ; make CC="$(alt)/cc -Qpath $(alt)" AS="$(alt)/as" \
	    F77="$(alt)/f77 -Qpath $(alt)" >> errs 2>&1
	touch $@

step2: step2libm
	cd $(libc) ; install libc.a $(alt)/libc.a ; ranlib $(alt)/libc.a
	cd $(libc)/suncsu ; cp *[01].o $(alt)
	cd $(libm) ; install libm.a $(alt)/libm.a ; ranlib $(alt)/libm.a
	touch $@

#
# step 3: Make new compiler passes with the new compiler
# and link them with the new library.
#

step3clean: step2
	cd $(compile) ; make clean
	cd $(cpp) ; make clean
	cd $(pcc) ; make clean
	cd $(c2) ; make clean
	cd $(as) ; make clean
	cd $(ld) ; make clean
	cd $(iropt) ; make clean
	cd $(f77pass1) ; make clean
	touch $@

step3cc: step3clean
	cd $(compile) ; make CC="$(newcc)"
	touch $@

step3cpp: step3cc
	cd $(cpp) ; make CC="$(newcc)"
	touch $@

step3pcc: step3cpp
	cd $(pcc) ; make CC="$(newcc)"
	touch $@

step3lint: step3pcc
	cd $(lint) ; make CC="$(newcc)"
	touch $@

step3c2: step3lint
	cd $(c2) ; make CC="$(newcc)"
	touch $@

step3as: step3c2
	cd $(as) ; make CC="$(newcc)"
	touch $@

step3ld: step3as
	cd $(ld) ; make CC="$(newcc)"
	touch $@

step3iropt: step3ld
	cd $(iropt) ; make CC="$(newcc)"
	touch $@

step3f77pass1: step3iropt
	cd $(f77pass1) ; make CC="$(newcc)"
	touch $@

#
# install new components...
#
step3:	step3f77pass1
	cd $(compile) ; install compile $(alt)/compile ; \
	    for i in ${CMDS}; do \
		(rm -f $(alt)/$$i; ln -s compile $(alt)/$$i)\
	    done
	cd $(cpp) ; install cpp $(alt)/cpp
	cd $(pcc) ; install comp $(alt)/ccom
	cd $(pcc) ; install cg $(alt)/cg
	cd $(pcc) ; install fort $(alt)/pc1
	cd $(c2) ; install c2 $(alt)/c2
	cd $(as) ; install as $(alt)/as
	cd $(ld) ; install ld $(alt)/ld
	cd $(iropt) ; install iropt $(alt)/iropt
	cd $(f77pass1) ; install f77pass1 $(alt)/f77pass1
	touch $@

#
# At this point, we should have achieved a 'fixed point'.
# To test this, repeat steps 2 and 3 and compare results.
#
step4clean: step3
	cd $(libc) ; mv libc.a libc.bak ; make clean > /dev/null 2>&1 ;
	cd $(libm) ; mv libm.a libm.bak ; make clean > /dev/null 2>&1 ;
	touch $@

step4libc: step4clean
	cd $(libc) ; make CC="$(alt)/cc -Qpath $(alt)" AS="$(alt)/as" \
	    >> errs 2>&1
	touch $@

step4libm: step4libc
	cd $(libm) ; make CC="$(alt)/cc -Qpath $(alt)" AS="$(alt)/as" \
	    F77="$(alt)/f77 -Qpath $(alt)" >> errs 2>&1
	touch $@

step4: step4libm
	cd $(libc) ; install libc.a $(alt)/libc.a ; ranlib $(alt)/libc.a
	cd $(libc)/suncsu ; cp *[01].o $(alt)
	cd $(libm) ; install libm.a $(alt)/libm.a ; ranlib $(alt)/libm.a
	touch $@

#
# ...rebuild the new components
#
step5clean: step4
	cd $(compile) ; make clean
	cd $(cpp) ; make clean
	cd $(pcc) ; make clean
	cd $(c2) ; make clean
	cd $(as) ; make clean
	cd $(ld) ; make clean
	cd $(iropt) ; make clean
	cd $(f77pass1) ; make clean
	touch $@

step5cc: step5clean
	cd $(compile) ; make CC="$(newcc)"
	touch $@

step5cpp: step5cc
	cd $(cpp) ; make CC="$(newcc)"
	touch $@

step5pcc: step5cpp
	cd $(pcc) ; make CC="$(newcc)"
	touch $@

step5lint: step5pcc
	cd $(lint) ; make CC="$(newcc)"
	touch $@

step5c2: step5lint
	cd $(c2) ; make CC="$(newcc)"
	touch $@

step5as: step5c2
	cd $(as) ; make CC="$(newcc)"
	touch $@

step5ld: step5as
	cd $(ld) ; make CC="$(newcc)"
	touch $@

step5iropt: step5ld
	cd $(iropt) ; make CC="$(newcc)"
	touch $@

step5f77pass1: step5iropt
	cd $(f77pass1) ; make CC="$(newcc)"
	touch $@

#
# ...and if the results differ, something is broken.
#
sanity: step5f77pass1
	cd $(compile) ; cmp compile $(alt)/compile
	cd $(cpp) ; cmp cpp $(alt)/cpp
	cd $(pcc) ; cmp comp $(alt)/ccom
	cd $(pcc) ; cmp cg $(alt)/cg
	cd $(pcc) ; cmp fort $(alt)/pc1
	cd $(c2) ; cmp c2 $(alt)/c2
	cd $(as) ; cmp as $(alt)/as
	cd $(ld) ; cmp ld $(alt)/ld
	cd $(iropt) ; cmp iropt $(alt)/iropt
	cd $(f77pass1) ; cmp f77pass1 $(alt)/f77pass1
	touch $@

clean:
	cd $(compile) ; make clean
	cd $(cpp) ; make clean
	cd $(pcc) ; make clean
	cd $(c2) ; make clean
	cd $(as) ; make clean
	cd $(ld) ; make clean
	cd $(iropt) ; make clean
	cd $(f77pass1) ; make clean
	rm -f sanity step*
	touch $@

install: sanity
	cd $(compile) ; make install DESTDIR=$(DESTDIR)
	for i in ${INSTALLCMDS}; do \
	    (rm -f $(DESTDIR)/$$i; ln -s /lib/compile $(DESTDIR)/$$i); \
	done
	cd $(cpp) ; make install DESTDIR=$(DESTDIR)
	cd $(pcc) ; make install DESTDIR=$(DESTDIR)
	cd $(lint) ; make install DESTDIR=$(DESTDIR)
	cd $(c2) ; make install DESTDIR=$(DESTDIR)
	cd $(as) ; make install DESTDIR=$(DESTDIR)
	cd $(ld) ; make install DESTDIR=$(DESTDIR)
	cd $(iropt) ; make install DESTDIR=$(DESTDIR)
	cd $(f77pass1) ; make install DESTDIR=$(DESTDIR)
	cd $(libc) ; make install DESTDIR=$(DESTDIR)
	cd $(libm) ; make install DESTDIR=$(DESTDIR)
