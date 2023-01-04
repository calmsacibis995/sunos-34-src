# makefile for documentation pertaining to diagnostics
# "make all" will make printed copies of all of these documents.
# @(#) Makefile 1.1 9/25/86

all:		autodefect color.diag cpu2.diag ether2.diag ffpusr keyboard\
		mem.diag menu sc.diag scsi.diag\
		sky.diag stdiag sysdiag sysgen video120.diag vmad.diag

autodefect:
		troff -t -me autodefect.txt | lpr -t -Pimagen

color.diag:
		troff -t -me color.diag.txt |  lpr -t -Pimagen

ffpusr:
		troff -t -me ffpusr.txt | lpr -t -Pimagen

keyboard:
		troff -t -me keyboard.txt | lpr -t -Pimagen

mem.diag:
		pr < mem.diag.doc | lpr -Pimagen 

menu:
		troff -t -me menu.txt |  lpr -t -Pimagen
	
sc.diag:
	-soelim sc.diag.msun | itroff  -msun 2>sc.diagtc.msun
	echo .UH CONTENTS > sc.diagPages.msun
	grep '^\.CE [CANU]' sc.diagtc.msun | \
		itroff -rP1 -msun sc.diagPages.msun -
	rm sc.diagtc.msun sc.diagPages.msun
#		troff -t -me sc.diag.txt |  lpr -t -Pimagen

video120.diag:
	-soelim video120.diag.msun | itroff  -msun 2>video120.diagtc.msun
	echo .UH CONTENTS > video120.diagPages.msun
	grep '^\.CE [CANU]' video120.diagtc.msun | \
		itroff -rP1 -msun video120.diagPages.msun -
	rm video120.diagtc.msun video120.diagPages.msun

scsi.diag:
		troff -t -me scsi.diag.txt |  lpr -t -Pimagen

sky.diag:
	-soelim sky.diag.msun | itroff  -msun 2>sky.diagtc.msun
	echo .UH CONTENTS > sky.diagPages.msun
	grep '^\.CE [CANU]' sky.diagtc.msun | \
		itroff -rP1 -msun sky.diagPages.msun -
	rm sky.diagtc.msun sky.diagPages.msun

vmad.diag:
	-soelim vmad.diag.msun | itroff  -msun 2>vmad.diagtc.msun
	echo .UH CONTENTS > vmad.diagPages.msun
	grep '^\.CE [CANU]' vmad.diagtc.msun | \
		itroff -rP1 -msun vmad.diagPages.msun -
	rm vmad.diagtc.msun vmad.diagPages.msun

cpu2.diag:
	-soelim cpu2.diag.msun | itroff  -msun 2>cpu2.diagtc.msun
	echo .UH CONTENTS > cpu2.diagPages.msun
	grep '^\.CE [CANU]' cpu2.diagtc.msun | \
		itroff -rP1 -msun cpu2.diagPages.msun -
	rm cpu2.diagtc.msun cpu2.diagPages.msun

ether2.diag:
	-soelim ether2.diag.msun | itroff  -msun 2>ether2.diagtc.msun
	echo .UH CONTENTS > ether2.diagPages.msun
	grep '^\.CE [CANU]' ether2.diagtc.msun | \
		itroff -rP1 -msun ether2.diagPages.msun -
	rm ether2.diagtc.msun ether2.diagPages.msun

stdiag:
	-soelim stdiag.msun | itroff  -msun 2>stdiagtc.msun
	echo .UH CONTENTS > stdiagPages.msun
	grep '^\.CE [CANU]' stdiagtc.msun | \
		itroff -rP1 -msun stdiagPages.msun -
	rm stdiagtc.msun stdiagPages.msun
#	troff -t -me stdiag.txt |  lpr -t -Pimagen

sysdiag:
		troff -t -me sysdiag.txt | lpr -t -Pimagen

sysgen:
		troff -t -me sysgen.txt | lpr -t -Pimagen

adaptec.txt:
		troff -t -me adaptec.txt | lpr -t -Pimagen

cdiag.txt:
		troff -t -me cdiag.txt | lpr -t -Pimagen

pdiag.txt:
		itroff -t -me pdiag.txt | lpr -t -Pimagen

primediag.txt:
		itroff -t -me primediag.txt | lpr -t -Pimagen

imemtest.txt:
		troff -t -me imemtest.txt | lpr -t -Pimagen

mouse.txt:
		troff -t -me mouse.txt | lpr -t -Pimagen

scdiag.txt:
		troff -t -me scdiag.txt | lpr -t -Pimagen

