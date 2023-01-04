#
# @(#)Makefile 1.1 86/09/25 SMI
#
FONTS = apl.r.10 cmr.r.14 cmr.b.14 gallant.r.19 		\
	screen.r.7 screen.r.11 screen.r.12 screen.r.13		\
	screen.r.14 screen.b.12 screen.b.14			\
	serif.r.10 serif.r.11 serif.r.12 serif.r.14 serif.r.16	\
	cour.r.10 cour.r.12 cour.r.14 cour.r.16 cour.r.18 cour.r.24 \
	cour.b.10 cour.b.12 cour.b.14 cour.b.16 cour.b.18 cour.b.24

all:
	
clean:
	
install:
	-for i in ${FONTS}; do \
		cp $$i ${DESTDIR}/usr/lib/fonts/fixedwidthfonts && \
		chmod 644 ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/$$i ; \
	done 
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/sail.r.6
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.b.7
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.r.7
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.b.8
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.r.8
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gallant.r.10
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/cmr.b.8
	-rm -f ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/cmr.r.8
	-ln -s screen.r.7  ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/sail.r.6
	-ln -s screen.b.12 ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.b.7
	-ln -s screen.r.12 ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.r.7
	-ln -s screen.b.14 ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.b.8
	-ln -s screen.r.14 ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gacha.r.8
	-ln -s gallant.r.19 ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/gallant.r.10
	-ln -s cmr.b.14		${DESTDIR}/usr/lib/fonts/fixedwidthfonts/cmr.b.8
	-ln -s cmr.r.14		${DESTDIR}/usr/lib/fonts/fixedwidthfonts/cmr.r.8
	cp README.install ${DESTDIR}/usr/lib/fonts/fixedwidthfonts/README

	
