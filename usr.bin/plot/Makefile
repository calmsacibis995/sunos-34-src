#
# @(#)Makefile 1.1 86/09/25 SMI; from UCB 4.7 85/11/11
#
CFLAGS=	-O
ALL=	tek t4013 t300 t300s t450 aedplot bgplot crtplot dumbplot gigiplot \
	hpplot hp7221plot implot atoplot plottoa vplot

all:	${ALL} debug

tek:	driver.o
	cc -o tek  driver.o -l4014 -lm

t4013:	driver.o
	cc -o t4013  driver.o -l4013 -lm

t300:	driver.o 
	cc -o t300 driver.o -l300 -lm

t300s:	driver.o 
	cc -o t300s driver.o -l300s -lm

t450:	driver.o 
	cc -o t450 driver.o -l450 -lm

vplot:	vplot.o chrtab.o
	cc -o vplot vplot.o chrtab.o

aedplot: driver.o
	cc -o aedplot driver.o -lplotaed

bgplot: driver.o
	cc -o bgplot driver.o -lplotbg -lm

crtplot: crtdriver.o crtplot.o
	cc -o crtplot crtdriver.o crtplot.o -lcurses -ltermcap -lm

dumbplot: driver.o
	cc -o dumbplot driver.o -lplotdumb -ltermcap -lm

gigiplot: driver.o
	cc -o gigiplot driver.o -lplotgigi -lm

hpplot: driver.o
	cc -o hpplot driver.o -lplot2648 -lm

hp7221plot: driver.o
	cc -o hp7221plot driver.o -lplot7221 -lm

implot: driver.o
	cc -o implot driver.o -lplotimagen -lm

atoplot: atoplot.o
	cc -o atoplot atoplot.o -lplot -lm

plottoa: plottoa.o
	cc -o plottoa plottoa.o

debug: debug.o
	cc -o debug debug.o

install: all
	-for i in ${ALL}; do \
		(install -s $$i ${DESTDIR}/usr/bin/$$i); done
	install -s vplot ${DESTDIR}/usr/bin
	install -c plot.sh ${DESTDIR}/usr/bin/plot

clean:
	rm -f *.o ${ALL} a.out core errs
