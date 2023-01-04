c - ffpusr.f
c - test program for skyffp
c
	program ffpusr
	include 'ffpusr.inc'
c - by default we run balls to the wall on automath test for board test
c - but if command is "d" versus "a" (from sysdiag), we sleep periodically.
c - or, if command is "z" (For ATN) then we sleep for the length of the
c - second argument we were called with.
	isleep=0
	pass=0
c 	write(*,101)
c 101	format('Sky: (c)1983-sky computers,inc.-v1.20',/,
c     1	'    @(#)ffpusr.f 1.1 9/25/86 Copyright Sun Microsystems')
c
c - do a call to find the system type
	call systype (i)
	VME=i
	if(i.lt.0) then
	    print *,'      ffpusr does not recognize the system type it is on.'
	    print *,'      check the systype routine in ffpc.c.'
	endif
c -- DEBUG
c	if(i.eq.0) then
c	    print *,'      ffpusr thinks this is a multibus system'
c	endif
c	if(i.gt.0) then
c	    print *,'      ffpusr thinks this is a VME-bus system'
c	endif
c
c - do a phys call to set up the i/o page
	call mapsky (i)
	if(i.lt.0) then
	    write (*,131) i
	    call exit(99)
	endif
	if(i.eq.0) then
	    write (*,132) i
	    call exit(99)
	endif
131	format ('	bad mapsky call',i8,' - must be super user to get phys')
132	format ('	bad mapsky call',i8,' - can not open device')

c 	102	format(' ? ',$)
c	5	write(*,102)
5	read(*,6) icode
6	format(a1)
c -- Next line commented out to eliminate extraneous newlines
c	write(*,7)
c 7	format(/)
408	if(icode.eq.'h') write(*,103)
	if(icode.eq.'h') go to 5
	if(icode.eq.'?') write(*,103)
	if(icode.eq.'?') go to 5
103	format (
     1	' valid function codes are:   ',/,
     1	'   a = automatic GO/NOGO diagnostics     ',/,
     1	'   d = Diagnose for SysDiag ',/,
     1	'   z = Diagnose for ATN ',/,
     1	'   r = register diagnostics    ',/,
     1	'   m = memory diagnostics      ',/,
     1	'   l = load ascii microcode into ffp ',/,
     1	'   i = initialize the ffp      ',/,
     1	'   t = test selected math functions     ',/,/,
     1	'   e = execute a pio sequence      ',/,
     1	'   p = pio communication       ',/,
     1	'   f = do a spfp ffp function       ',/,
     1  '   s = do a special function   ',/,
     1	'   b = base address modifier ',/,
     1  '   c = user context swap  ',/,
     1  '   v = verbose ',/,
     1  '   n = not verbose ',/,
     1  '   q = quit - return to o.s.   ',/)
	if(icode.eq.'q') call exit(0)
c -- Next line commented out to eliminate redundant echoing
c	write(*,8) icode
c 8	format(/,' ',a1,' initiated')
	if(icode.eq.'a') then
	    isleep = 0
	    verbose = 1
            call autotst
	else if(icode.eq.'d') then
	    isleep = -1
            call autotst
	else if(icode.eq.'z') then
	    if(verbose.eq.1) then
	       print *,'sky: ATN command entered'
	    endif
	    read(*,96) isleep
96	    format(i3)
	    if(verbose.eq.1) then
	       write(*,97) isleep
97	       format(/,'sky: Sleep value entered ',i3,' seconds')
	    endif
	    call autotst
	else if(icode.eq.'m') then
	    call memtst
	else if(icode.eq.'r') then
	    call regtst
	else if(icode.eq.'l') then
	    call loaduc
	else if(icode.eq.'e') then
	    call execut
	else if(icode.eq.'p') then
	    call piocom
	else if(icode.eq.'t') then
	    call tmath
	else if(icode.eq.'i') then
	    call fpinit
	else if(icode.eq.'b') then
	    call basmod
	else if(icode.eq.'f') then
	    call ffpfun
	else if(icode.eq.'s') then
	    call spcfun
	else if(icode.eq.'c') then
	    call swaper
	else if(icode.eq.'k') then
	    loadtest = 1
	else if(icode.eq.'v') then
	    verbose = 1
	    print *,'verbose on'
	else if(icode.eq.'n') then
	    if(verbose.eq.1) then
	        print *,'verbose off'
	        verbose = 0
	    endif
	else
	    if(icode.ne.'0') then
	        print *,' what ?'
	    endif
	endif
	go to 5
	end
	subroutine memtst
c - memory test
	include 'ffpusr.inc'
	integer*2	lomem,himem,memsiz,ones(2,16),ierr,icnt,iclr
	integer*4	bigone(16)
	equivalence	(bigone(1),ones(1,1))
	data	lomem/4096/,himem/8191/
	data bigone/1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,
     1	16384,32768/
	data	iclr/0/
	ierr = 0
c
	if(verbose.eq.1) then
	    print *,' ==>memtest: testing microcode RAM'
	endif
c
c - reset and halt the machine
	call outw (stcreg,reset)
	call outw (stcreg,ihalt)
c
c - walking one test
	do 50	i=lomem,himem
c - write the memory address into the command register
	icnt = i
	call outw (comreg,icnt)
c - do the low microcode word first
	iclr = 0
	call outw (mc2reg,iclr)
	do 51	j=1,16
	im1=j-1
	call outw (mc1reg,ones(2,j))
	call inw  (mc1reg,iword)
	if(ones(2,j).ne.iword) then
	cword(1) = wtohx(ones(2,j))
	cword(2) = wtohx(iword)
	cword(3) = wtohx(icnt)
	    write(*,105) cword(1),cword(2),im1,cword(3)
	    ierr = ierr + 1
	    if(ierr.gt.10) go to 888
	endif
51	continue
c - do the high microcode word next
	call outw (mc1reg,iclr)
	do 53	j=1,16
	im1=j+15
	call outw (mc2reg,ones(2,j))
	call inw  (mc2reg,iword)
	if(ones(2,j).ne.iword) then
	cword(1) = wtohx(ones(2,j))
	cword(2) = wtohx(iword)
	cword(3) = wtohx(icnt)
	    write(*,105) cword(1),cword(2),im1,cword(3)
	    ierr = ierr + 1
	    if(ierr.gt.10) go to 888
	endif
53	continue
105	format(' <memtst>error: failed memory walking ones test',/,
     1	16x,'wrote ',a4,5x,'read ',a4,5x,'at bit ',i2,5x,
     1	'location ',a4)
50	continue
c - unique memory test - fill the memory first
	ierr = 0
	do 151 i=lomem,himem
	icnt = i
	call outw (comreg,icnt)
	iword=i+i
	call outw (mc1reg,iword)
	iword=iword+1
	call outw (mc2reg,iword)
151	continue
c - read the memory and check against itself
	do 152	i=lomem,himem
	icnt = i
	call outw (comreg,icnt)
	iword=i+i
	call inw  (mc1reg,jword)
	if(iword.ne.jword) then
	cword(1) = wtohx(iword)
	cword(2) = wtohx(jword)
	cword(3) = wtohx(icnt)
	    write(*,106)cword(1),cword(2),cword(3)
	    ierr = ierr +1
	    if(ierr.gt.10) go to 888
	endif
	iword=iword+1
	call inw  (mc2reg,jword)
	if(iword.ne.jword) then
	cword(1) = wtohx(iword)
	cword(2) = wtohx(jword)
	cword(3) = wtohx(icnt)
	    write(*,106)cword(1),cword(2),cword(3)
	    ierr = ierr +1
	    if(ierr.gt.10) go to 888
	endif
152	continue
106	format(' <memtst>error: failed uniqueness test',/,
     1	16x,'wrote ',a4,5x,'read ',a4,5x,'at location ',a4)
c - zero fill the memory
	iword = 0
	do 252 i=lomem,himem
	icnt = i
	call outw (comreg,icnt)
	call outw (mc1reg,iword)
	call outw (mc2reg,iword)
252	continue
c - un-halt the machine
	call outw (stcreg,runenb)
	if(ierr.eq.0) then
	    return
	else
	    print *,' end of memory test ',ierr,'  errors'
	    return
	endif
c - abort test
888	print *,' stopping memory test - >10 errors'
	return
	end
	
	subroutine regtst
c - register test
	include 'ffpusr.inc'
	integer*2	ierr, andmsk
c
	if(verbose.eq.1) then
	    print *,' ==>regtest: testing registers'
	endif
c			0x80 bit (Reset) always reads as 1
c	if(VME.eq.0) then
c			240 is 0xF0
c		andmsk= 240
c	else
c			112 is 0x70
		andmsk= 112
c	endif
	ierr = 0
c
c - test the stcreg
	call outw (stcreg,reset)
c - - - ihalt/xx0x
	call outw (stcreg,ihalt)
	call inw  (stcreg,iword)
	if(iandw(iword,andmsk).ne.ihalt) then
	    cword(1) = wtohx(iword)
	    write(*,700)cword(1)
700	    format('  error:stcreg/ihalt /xx0x',a4)
	    ierr = 1
	endif
c - - - intenb/xx1x
c	call outw (stcreg,intenb)
c	call inw  (stcreg,iword)
c	if(iandw(iword,andmsk).ne.intenb) then
c	    cword(1) = wtohx(iword)
c	    write(*,701)cword(1)
c 701	   format(,'  error:stcreg/intenb/xx1x',a4)
c	    ierr = 1
c	endif
c - - - single/xx2x
	call outw (stcreg,single)
	call inw  (stcreg,iword)
	if(iandw(iword,andmsk).ne.single) then
	    cword(1) = wtohx(iword)
	    write(*,702)cword(1)
702	    format('  error:stcreg/single/xx2x',a4)
	    ierr = 1
	endif
c - - - runenb/xx4x
	call outw (stcreg,runenb)
	call inw  (stcreg,iword)
	if(iandw(iword,andmsk).ne.runenb) then
	    cword(1) = wtohx(iword)
	    write(*,703)cword(1)
703	    format('  error:stcreg/runenb  /xx4x',a4)
	    ierr = 1
	endif
c - The reset bit is not readable
c - - - reset/xx8x
	call outw (stcreg,reset)
	call inw  (stcreg,iword)
	if(iandw(iword,andmsk).ne.ihalt) then
	    cword(1) = wtohx(iword)
	    write(*,704)cword(1)
704	    format('  error:stcreg/reset /xx8x',a4)
	    ierr = 1
	endif
c
c - walk a bit up the command register
	iword=1
c - halt the machine
	call outw (stcreg,ihalt)
	do 10 i=1,13
	im1=i-1
	if(i.gt.1) iword=iword*2
	call outw (comreg,iword)
	call inw  (comreg,jword)
	if(iword.eq.jword) go to 10
	ierr = 2
10	continue
c - reset the machine
	call outw (stcreg,reset)
c - - - write a message
	if(ierr.eq.0) then
	else if(ierr.eq.1) then
	    print *,'          : stcreg failed '
	else if(ierr.eq.2) then
	    print *,'          : comreg failed '
	endif
	return
	end
	subroutine loaduc
	include 'ffpusr.inc'
	integer*2	ucode2(2),vcode2(2)
	integer*2	loaddr,hiaddr,addres,nwords
	integer*4	vcode
	equivalence	(ucode,ucode2(1))
	equivalence	(vcode,vcode2(1))
	nwords=0
	loaddr=4096
c - halt the machine
	call outw (stcreg,ihalt)
c - file load
	fname='sky.ucode'
c	write(*,121)
121	format(' enter microcode ram file name <yyyyyyyy.ram>   ',$)
c	read(*,122) fname
122	format(a12)
	open(unit=10,file=fname,status='old',err=888)
	rewind 10
c
c - read and load the linked ram microcode
100	read(10,124,err=889,end=998) cword(1),clong(1)
124	format(1x,a4,1x,a8)
	addres = hxtow(cword(1),jerr)
	ucode  = hxtol(clong(1),jerr)
	nwords=nwords+1
c - write the data
	call outw (comreg,addres)
	call outw (mc1reg,ucode2(2))
	call outw (mc2reg,ucode2(1))
c - verify the data
	call inw  (mc1reg,vcode2(2))
	call inw  (mc2reg,vcode2(1))
	if(ucode.ne.vcode) then
	    cword(1) = wtohx(addres)
	    clong(1) = ltohx(ucode)
	    clong(2) = ltohx(vcode)
	    write(*,109) cword(1),clong(1),clong(2)
	endif
	hiaddr=addres
c - read some more microcode
	go to 100

888	write(*,1000) '  error: opening file'
	print *,'  error: opening file'
	go to 999
889	write(*,1000) '  error: reading file'
	print *,'  error: reading file'
998	continue
	close(unit=10,status='keep')
	cword(1) = wtohx(nwords)
	cword(2) = wtohx(loaddr)
	cword(3) = wtohx(hiaddr)
	write(*,110) cword(1),cword(2),cword(3)
	call outw (stcreg,runenb)
999	continue
110	format(' loading done:wrote ',a4,' words to address '
     1	,a4,' to ',a4)
109	format('  error: verifying at address ',a4,
     1	'  wrote ',a8,'   read ',a8)
	return
	end
	subroutine execut
	include 'ffpusr.inc'
	character*30	coment(400)
	character*6	seqnam(400)
	character*1	seqact(400)
	integer*2	seqdat(400),seqadr(400),nseq
	write(*,101)
101	format(' ==>execute a pio sequence ')
	write(*,102)
102	format(/,' enter sequence type ( n(ew)/o(ld)/f(ile) )   ',$)
	read(*,103)name
103	format(a6)
	if(name.eq.'o     '.or.name.eq.'f     ') go to 100
c
c - new sequence - read from user console
	write(*,104)
104	format(/,' in response to  "??" enter pio data',/,
     1	'  action(r/w/q),register name,data, and comment in'
     1	,/,'  <a1,1x,a6,1x,z4,a30 fmt> format'
     1	,/,'  --use "q"  on action to end the sequence--',/)
	nseq=0
	i=1
10	write(*,105) i
105	format(' seq=',i3,'  ?? ',$)
	read(*,106) seqact(i),seqnam(i),cword(1),coment(i)
106	format(a1,1x,a6,1x,a4,a30)
	if(seqact(i).eq.'q'.or.seqnam(i).eq.'quit  ') go to 201
	seqdat(i) = hxtow(cword(1),jerr)
	do 120 j=1,6
120	if(seqnam(i).eq.rgname(j)) go to 125
	write(*,109) rgname
109	format('  error: bad register name - valid ones are:',/,
     1	6(3x,a6))
	go to 10
125	seqadr(i)=rgport(j)
	nseq=nseq+1
	i=i+1
	if(i.gt.400) go to 889
	go to 10
c
c - sequence exists - so just execute
100	continue
	if(name.eq.'o     ') go to 201
	write(*,202)
202	format(' enter file name    <yyyyyyyy.zzz>              ',$)
	read(*,203) fname
203	format(a12)
	open(file=fname,unit=20,status='old',err=810)
	go to 215
810	print *,'  error: file not in directory'
	go to 999
215	i=1
	nseq=0
220	read(20,106,end=209) seqact(i),seqnam(i),cword(1),coment(i)
	seqdat(i) = hxtow(cword(1),jerr)
	do 320 j=1,6
320	if(seqnam(i).eq.rgname(j)) go to 325
325	seqadr(i)=rgport(j)
	nseq=nseq+1
	i=i+1
	go to 220
209	continue
	close(unit=20,status='keep')
c
c - sequence exists
201	continue
	write(*,107)
107	format(' sequence dump option ( n(one)/t(erminal)/f(ile) ) ',$)
	read (*,103) name
	if(name.eq.'n     ') go to 200
c - dump the sequence
	if(name.eq.'f     ') go to 230
	do 157 i=1,nseq
	cword(1) = wtohx(seqdat(i))
	cword(2) = wtohx(seqadr(i))
157	write(*,108) i,seqact(i),seqnam(i),cword(2),cword(1),
     1	coment(i)
108	format(' seq=',i3,' act=',a1,' reg=',a6,' adr=',a4,
     1	' data=',a4,2x,a30)
	go to 240
230	write(*,231)
231	format(' enter new file name        <yyyyyyyy.zzz>      ',$)
	read(*,203) fname
	open(file=fname,unit=20,status='new',err=883)
	go to 243
883	print *,'  error:file already exists'
	go to 230
243	do 234 i=1,nseq
	cword(1) = wtohx(seqdat(i))
234	write(20,106) seqact(i),seqnam(i),cword(1),coment(i)
	close(unit=20,status='keep')
240	continue
	write(*,110)
110	format(' execute this code  (y/n/v)  ? ',$)
	read (*,103) name
	if(name.eq.'n     ') go to 999
c
c - loop on sequence
200	continue
c - halt the machine
c	call outw(stcreg,ihalt)
	write(*,111)
111	format(' enter loop counter (-1=infinite)  ',$)
	read(*,*) nloop
	if(nloop.lt.0) go to 211
	do 210	j=1,nloop
	do 210	i=1,nseq
	if(seqact(i).eq.'w')call outw (seqadr(i),seqdat(i))
	if(seqact(i).eq.'r')call inw  (seqadr(i),seqdat(i))
	if(name.eq.'v     ') then
	    cword(1) = wtohx(seqadr(i))
	    cword(2) = wtohx(seqdat(i))
	    write(*,167) seqact(i),cword(1),cword(2)
167	    format('  verify ==> act=',a1,'  adr=',a4,'  data=',a4)
	endif
210	continue
	go to 999
c - infinate loop
211	continue
	do 212	i=1,nseq
	if(seqact(i).eq.'w')call outw (seqadr(i),seqdat(i))
	if(seqact(i).eq.'r')call inw  (seqadr(i),seqdat(i))
212	continue
	go to 211
c - error reporting
889	print *,'  error: exceeded limit of 400 actions'
999	continue
	return
	end
c
	subroutine fpinit
c - initialize the ffp board
c
	include 'ffpusr.inc'
	integer*2 start0,start1
	data	  start0,start1 /4096,4097/
c - - - reset
	call outw(stcreg,reset)
    	call outw(comreg,start0)
    	call outw(comreg,start0)
	call outw(comreg,start1)
c - - - run mode
	call outw(stcreg,runenb)
	return
	end
c
	subroutine basmod
	include 'ffpusr.inc'
	write(*,101)
101	format('  enter the base address of the ffp in hex 4   ',$)
	read (*,102) cword(1)
102	format(a4)
	nbase = hxtow(cword(1),ierr)
	do 10 i=1,6
10	rgport(i) = nbase + ((i-1) * 2)
	return
	end
c
	subroutine ffpfun
c - perform a ffp function
c
	include 'ffpusr.inc'
	real*4		op1,op2,op3
	integer*2	iop1(2),iop2(2),iop3(2),nops
	equivalence	(op1,iop1(1)),(op2,iop2(1)),(op3,iop3(1))
	write(*,101)
101	format(' ==>ffpfun: function test  ',/)
102	write(*,103)
103	format('  enter opcode in 4hex fmt.  ',$)
	read(*,116) cword(1)
116	format(a4)
	opcode = hxtow(cword(1),jerr)
	if(opcode.lt.4097) go to 102
c
c - send out the function code
	call outw (comreg,opcode)
c
c - read the stcreg and find out what to do next
12	call inw (stcreg,iword)
c - is i/o available  ??????
	if(iword.lt.0) then
c - i/o available, now check the i/o direction bit - set if 68k cpu read
	    if(iandw(iword,iodir).eq.0) then
c - write data to the skyffp
		write(*,107)
		read *, op1
		call outw(dt1reg,iop1(1))
		call outw(dt2reg,iop1(2))
		clong(1) = ltohx(op1)
		write(*,108) op1,clong(1)
	    else
c - read data from the skyffp
		call inw(dt1reg,iop2(1))
		call inw(dt2reg,iop2(2))
		clong(2) = ltohx(op2)
		write(*,109) op2,clong(2)
	    endif
	    go to 12
	else
c
c - check if skyffp is idle
	    if (iandw(iword,16384).eq.1) then
	        cword(1) = wtohx (iword)
	        write(*,110)
	    endif
	endif
107	format('  enter real input operand   ',$)
108	format('             input operand = ',e14.6,5x,a8)
109	format('            output operand = ',e14.6,5x,a8)
110	format('  task complete  -  stcreg = ', 23x,a4)
	return
	end
c
    	subroutine piocom
	include 'ffpusr.inc'
	write(*,101)
101	format(' ==> pio communication')
	write(*,14)
14	format(' enter action(r/w/q),reg-name,& data (?? prompt)'
     1	,/,'   format <a1,1x,a6,1x,a4> ',/)
5	write(*,15)
15	format(' ?? ',$)
6	read(*,16)icode,name,cword(1)
16	format(a1,1x,a6,1x,a4)
	if(icode.eq.'q'.or.icode.eq.' ') return
	iword = hxtow(cword(1),jerr)
	do 10 i=1,6
10	if(name.eq.rgname(i)) go to 20
	write(*,100)rgname
100	format(' register name does not match list:',/,6(3x,a6))
	go to 5
20	continue
	if(icode.eq.'w') call outw(rgport(i),iword)
	if(icode.eq.'r') call inw (rgport(i),iword)
	cword(1) = wtohx(iword)
	call inw (stcreg,iword)
	cword(2) = wtohx (iword)
	write(*,19) name,cword(1),cword(2)
19	format(' <piocom>',a6,'=',a4,'   stcreg=',a4)
	go to 5
	end
	block data ffpdat
c
	include 'ffpusr.inc'
c
	data rgname	/'comreg','stcreg','dt1reg','dt2reg',
     1			 'mc1reg','mc2reg'/
	data rgport	/0,2,4,6,8,10/
	data ascii	/'0','1','2','3','4','5','6','7','8','9',
     1			 'a','b','c','d','e','f'/
	data ascmap	/0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15/
	data	ihalt/0/,intenb/16/,single/32/,runenb/64/,sngrun/96/
	data	reset/128/,iodir/8192/,idle/16384/,iordy/-32768/
	data	cmbuff/8*4195/
	data	iold/1/
	data	dtbuff/64*0.0/
c
	data opname/
     1		 'addition        ',
     1		 'subtraction     ',
     1		 'multiplication  ',
     1		 'division        ',
     1		 'sine            ',
     1		 'cosine          ',
     1		 'tangent         ',
     1		 'func 8          ',
     1		 'func 9          ',
     1		 'func 10         ',
     1		 'func 11         ',
     1		 'func 12         ',
     1		 'func 13         ',
     1		 'func 14         ',
     1		 'func 15         ',
     1		 'func 16         '
     1	/
c
	end
	subroutine tmath

	include 'ffpusr.inc'
	integer*2 ifunc
	integer*4 thresh,nran,ndiff,jmax,power
	character*1 functions(16)
	data	functions/'+','-','*','/','s','c','t',9*'X'/
c
	write(*,101)
101	format(' ==>tmath : selected function test  ')
102	write(*,203)
203	format('  enter function symbol (+,-,*,/,s,c,or t)    ',$)
	read (*,204) icode
204	format(a1)
	ifunc = jindex(icode,functions)
	if(ifunc.eq.-1) then
	    print *,'Unknown function symbol'
	    goto 102
	endif
	write(*,208)
208	format('  enter number of bit difference allowed      ',$)
	read *,thresh
	write(*,206)
206	format('  enter the number of calculations to do      ',$)
	read(*,*) nran
	write(*,105)
105	format('  enter upper exponent (maximum = 38)         ',$)
	read *, power
	if(power.gt.38) power = 38
	jmax = 0
	call domath(ifunc,nran,power,thresh,ndiff,jmax)
	if(ndiff.eq.0) then
	    write(*,136)
136	format('  no differences between host and skyffp')
	else
	    clong(1) = ltohx(ndiff)
	    clong(2) = ltohx(jmax)
	    write (*,137) ndiff, jmax
	    write (0,137) ndiff, jmax
137	format(2x,i8,' numbers were different with a max of ',i8,' bits')
	endif
	return
	end

	integer function jindex(char,table)
	character*1 char,table(16)
	do 10 i = 1,16
	    if (table(i).eq.char) then
		jindex = i
		return
	    endif
10	continue
	jindex = -1
	return
	end

c - ran.for
c
c - random number function
	function ran(x)
	real*4	seed,base,ran,x
	data seed/0.2510637948/
	data base/29.0/
	ran=base*seed
	ran=ran-ifix(ran)
	seed=ran
	return
	end
c
c
	subroutine swaper
	include 'ffpusr.inc'
	write(*,101) iold
101	format('  current user is',i2,' - enter new user (1-8)  ',$)
	read *,inew
	if(inew.lt.1) inew = 1
	if(inew.gt.8) inew = 8
	call doswap(inew)
	return
	end
c
	subroutine swap
	include 'ffpusr.inc'
	inew=iold+1
	if(inew.lt.1) inew = 1
	if(inew.gt.8) inew = 1
	call doswap(inew)
	return
	end
c
	subroutine doswap(inew)
c
c - doswap is a simulation of the context swapping algorithm
c
c - two buffers need to be set up by the operating system
c - - - buffer 1:
c		integer*4 dtbuff(8,max_number_of_users)
c
c	this is a long word (32 bit) buffer initialized at boot time to
c	all zeros.  used for holding the 8 saved registers of the ffp
c	processor.  eight long words are saved and/or restored for
c	each user swap that occurs.
c
c - - - buffer 2:
c		integer*2 cmbuff(max_number_of_users)
c
c	this is a short (16 bit) word buffer initialized at boot time
c	to a hex value of 1063 which corresponds to a nop command to the
c	ffp processor.  the saved users ffp program counter is saved in
c	this buffer.  once a user has been restored this is set to a ffp nop
c	value of 1063 hex.
c
c
c - pertinent constants:
c
c	save	equ	0x1040
c	restor	equ	0x1041
c	nop	equ	0x1063
c	ihalt	equ	0x0000
c	runenb	equ	0x0040
c	sngrun	equ	0x0060
c	iordy	equ	0x8000
c	idle	equ	0x4000
c	iodir	equ	0x2000
c
c
c - user context swapping
c
	include 'ffpusr.inc'
c
	integer*2	status,dummy
	integer*2	save,restor,nop
	data	dummy/0/
	data	save,restor,nop /4160,4161,4195/
c
100	continue
c - - - - - - - - - - - - - - - - - - - - - - - - - -
c - read the status register - get the machine state
c - - - - - - - - - - - - - - - - - - - - - - - - - -
c
c - idle - i/o ready test loop
c - - - note: the 100 looks is arbitrary.  the limit should be
c - - - set so that 500 microseconds is exausted.  if the time
c - - - is exausted then a fatal error condition has occured with
c - - - that particular users code.  The offending task should be
c - - - aborted.  the 500 microsecond time is much longer than ever
c - - - anticipated and is used as a safe upper limit, considering
c - - - that a user programmed microcode function in the ffp could
c - - - be quite extensive.
c
	do 20 i=1,100
	call inw (stcreg,status)
c - check for idle state - then for i/o ready state
	if ((iandw(status,idle)).gt.0) then
	    cmbuff(iold) = nop
	    go to 200
	endif
	if ((iandw(status,iordy)).ne.0) go to 25
20	continue
c - timed out - report the error - abort processing of swapper
	write(*,1999)
1999	format(' swaper: time out error ')
	return
25	continue
c
c - i/o is ready - is it a read or write ?
c
c - first set the machine to single step/run mode
	call outw (stcreg,sngrun)
c - check i/o direction bit
	if ((iandw(status,iodir)).ne.0) then
	    call inw (dt1reg,dummy)
	else
	    call outw(dt1reg,dummy)
	endif
c - check again since data may have been a long word
	call inw (stcreg,status)
	if ((iandw(status,iordy)).ne.0) then
	    if ((iandw(status,iodir)).ne.0) then
		call inw (dt1reg,dummy)
	    else
		call outw (dt1reg,dummy)
	    endif
	endif
c - - - - - - - - - - - - - - - - - - -
c - read and save the command register
c - - - - - - - - - - - - - - - - - - -
	call inw (comreg,cmbuff(iold))
c - decrement by 1 : command register is actually ffp program counter
	cmbuff(iold) = cmbuff(iold) - 1
c - - - - - - - - - - - - -
c - re-initialize the ffp
c - - - - - - - - - - - - -
	call fpinit
200	continue
c - send a nop : sets ffp in a clean mode
	call outw (comreg,nop)
c - - - - - - - - - - - - - - - -
c - do the context save function
c - - - - - - - - - - - - - - - -
c
c - - - send the context save opcode
	call outw (comreg,save)
c - - - read the 8 long words
	do 30 i=1,8
	call inl (dt1reg,dtbuff(i,iold))
30	continue
c
c - - - - - - - - - - - - - - - - -
c - do the context restore function
c - - - - - - - - - - - - - - - - -
c
c - - - send the context restore command
	call outw (comreg,restor)
c - - - write the 8 long words
	do 35 i=1,8
	call outl (dt1reg,dtbuff(i,inew))
35	continue
c - restore the command register
	call outw (comreg,cmbuff(inew))
c - set the old user number to the new user number
	iold = inew
c
c - that's all for context swapping
c
	return
	end
c
	subroutine spcfun
c - perform a special ffp function
c
	include 'ffpusr.inc'
	real*8		dop1,dop2,dop3
	real*4		op1(2),op2(2),op3(2)
	integer*2	iop1(4),iop2(4),iop3(4),nops
	integer*4	jop1(2),jop2(2),jop3(2)
	equivalence (op1(1),iop1(1)),(op2(1),iop2(1)),(op3(1),iop3(1))
	equivalence (op1(1),jop1(1)),(op2(1),jop2(1)),(op3(1),jop3(1))
	equivalence (op1(1),dop1   ),(op2(1),dop2   ),(op3(1),dop3   )
	write(*,101)
101	format(' ==>spcfun: function test  ',/)
102	write(*,103)
103	format('  enter opcode in 4hex fmt.  ',$)
	    read(*,8116) cword(1)
	opcode = hxtow(cword(1),ierr)
	if(opcode.lt.4097) go to 102
8116	format(a4)
c
c - send out the function code
	call outw (comreg,opcode)
c
c - read the stcreg and find out what to do next
12	call inw (stcreg,iword)
c - is i/o available  ??????
	if(iword.lt.0) then
c - i/o available, now check the i/o direction bit - set if host cpu read
	    if(iandw(iword,iodir).eq.0) then
c - write data to the skyffp
		write(*,207)
		read(*,201) icode
		if     (icode.eq.'h') then
		    write(*,209)
		else if(icode.eq.'s') then
		    write(*,107) 'spfp'
		    read *, op1(1)
		    call outl(dt1reg,op1(1))
		    clong(1) = ltohx(jop1(1))
		    write(*,108) op1(1),clong(1)
		else if(icode.eq.'d') then
		    write(*,107) 'dpfp'
		    read *, dop1
		    call outl(dt1reg,op1(1))
		    call outl(dt1reg,op1(2))
		    clong(1) = ltohx(jop1(1))
		    clong(2) = ltohx(jop1(2))
		    write(*,109) dop1,clong(1),clong(2)
		else if(icode.eq.'l') then
		    write(*,107) 'long'
		    read *, jop1(1)
		    call outl(dt1reg,jop1(1))
		    clong(1) = ltohx (jop1(1))
		    write(*,110) jop1(1),clong(1)
		else if(icode.eq.'w') then
		    write(*,107) 'word'
		    read *,iop1(1)
		    call outw(dt1reg,iop1(1))
		    cword(1) = wtohx(iop1(1))
		    write(*,111) iop1(1),cword(1)
		else
		    write(*,209)
		endif
	    else
c - read data from the skyffp
		write(*,208)
		read(*,201) icode
		if     (icode.eq.'h') then
		    write(*,209)
		else if(icode.eq.'s') then
		    call inl(dt1reg,op1(1))
		    clong(1) = ltohx(jop1(1))
		    write(*,308) op1(1),clong(1)
		else if(icode.eq.'d') then
		    call inl(dt1reg,op1(1))
		    call inl(dt1reg,op1(2))
		    clong(1) = ltohx(jop1(1))
		    clong(2) = ltohx(jop1(2))
		    write(*,309) dop1,clong(1),clong(2)
		else if(icode.eq.'l') then
		    call inl(dt1reg,jop1(1))
		    clong(1) = ltohx (jop1(1))
		    write(*,310) jop1(1),clong(1)
		else if(icode.eq.'w') then
		    call inw(dt1reg,iop1(1))
		    cword(1) = wtohx(iop1(1))
		    write(*,311) iop1(1),cword(1)
		else
		    write(*,209)
		endif
	    endif
	    go to 12
	else
c
c - check if skyffp is idle
	    if (iandw(iword,idle).eq.1) then
		cword(1) = wtohx(iword)
		write(*,8110) cword(1)
	    endif
	endif
201	format(a1)
207	format('  enter input data type      ',$)
208	format('  enter output data type     ',$)
209	format('  s=spfp,d=dpfp,l=long,w=word,h=help ')
107	format('  enter ',a4,' input operand   ',$)
108	format('        spfp input operand = ',e14.6,5x,a8)
109	format('        dpfp input operand = ',e14.6,5x,2a8)
110	format('        long input operand = ',i14,5x,a8)
111	format('        word input operand = ',i14,5x,a4)
308	format('       spfp output operand = ',e14.6,5x,a8)
309	format('       dpfp output operand = ',e14.6,5x,2a8)
310	format('       long output operand = ',i14,5x,a8)
311	format('       word output operand = ',i14,5x,a4)
8110	format('  task complete  -  stcreg = ', 23x,a4)
	return
	end
c
        subroutine autotst
        include 'ffpusr.inc'
1       pass = pass + 1
	call fpinit
        call regtst
        call memtst
        fname='sky.ucode'
        call autoload
        call fpinit
        call automath
c -	If  isleep  > 1 then we're running for ATN, need to loop forever
	if (isleep.gt.0) then
		if(verbose.eq.1) then	
	           write(*,300) pass
300		   format(/,'sky: End of pass ',i3)
		endif
		goto 1
	endif
        return
        end
c
	subroutine autoload
	include 'ffpusr.inc'
	integer*2	ucode2(2),vcode2(2)
	integer*2	loaddr,hiaddr,addres,nwords
	integer*4	vcode
	integer*2	tmp, HASHS2A1
	equivalence	(ucode,ucode2(1))
	equivalence	(vcode,vcode2(1))
c
	if (verbose.eq.1) then
	    print *,' ==>autoload: loading microcode'
	endif
c
c -- This is a # character.  If the first character in a line is this
c -- symbol, skip the line.  Allows comments in the microcode file.
	HASHS2A1 = 8992
	nwords=0
	loaddr=4096
c - halt the machine
	call outw (stcreg,ihalt)
c - file load
	open(unit=10,file=fname,status='old',err=888)
	rewind 10
c
c - read and load the linked ram microcode
100	read(10,124,err=889,end=998) tmp, cword(1),clong(1)
124	format(a1,a4,1x,a8)
	if (tmp .eq. HASHS2A1) goto 100
	addres = hxtow(cword(1),jerr)
	ucode  = hxtol(clong(1),jerr)
	nwords=nwords+1
c - write the data
	call outw (comreg,addres)
	call outw (mc1reg,ucode2(2))
	call outw (mc2reg,ucode2(1))
c - verify the data
	call inw  (mc1reg,vcode2(2))
	call inw  (mc2reg,vcode2(1))
	if(ucode.ne.vcode) then
	    cword(1) = wtohx(addres)
	    clong(1) = ltohx(ucode)
	    clong(2) = ltohx(vcode)
	    write(*,109) cword(1),clong(1),clong(2)
	endif
	hiaddr=addres
c - read some more microcode
	go to 100
888	print *,'  error: opening file'
	go to 999
889	print *,'  error: reading file'
998	continue
	close(unit=10,status='keep')
	cword(1) = wtohx(nwords)
	cword(2) = wtohx(loaddr)
	cword(3) = wtohx(hiaddr)
c	write(*,110) cword(1),cword(2),cword(3)
	call outw (stcreg,runenb)
999	continue
110	format('            : loading done:wrote ',a4,' words to address '
     1	,a4,' to ',a4)
109	format('       error: verifying at address ',a4,
     1	'  wrote ',a8,'   read ',a8)
	return
	end
c
	subroutine automath
	include 'ffpusr.inc'
	integer*2	ifunc
	integer*4	iout(16),jmax(16),nran,sumran,power,maxpower
c
	if(verbose.eq.1) then
	    print *,' ==>automath: testing math'
	endif
c
	sumran=0
c - test how many functions of math?
	nfunc=5
c - maximum exponent of values to be tested is
	power=0
	if(loadtest.eq.1) then
	    maxpower = 1
	else
	    maxpower = 37
	endif
c - number of random numbers per function, initial value
	nran=10
c - number of bit difference allowed and worst error so far
	do 104 j = 1,nfunc
	    iout(j)=1
	    jmax(j)=0
104	continue
	iout(5)=10
c
c -- Beginning of loop over power
	do 100 power = 1,maxpower
c
c -- Perform the math tests, but don't do sines for powers larger than 4
	do 409 ifunc=1,nfunc
	    if (ifunc.ne.5  .or.  (power.lt.4 .and. power.gt.-4)) then
	        call domath(ifunc,nran,power,iout(ifunc),ndiff,jmax(ifunc))
	    endif
409	continue
c
	if (isleep.eq.-1 .and. loadtest.ne.1) then
		call slumber
	endif
	if (isleep.gt.0) then
		if(verbose.eq.1) then
	           write(*,420) isleep
420		   format(/,'sky: Sleeping for ',i3,' seconds')
		endif
c		call sleep(isleep)
		call slumber
		if(verbose.eq.1) then
		   print *,'sky: Sleep ended'
		endif
	endif
	sumran=sumran+nran
	call swap
	nran=nran*1.1
100	continue
c -- end of main loop

	do 489 jj=1,nfunc
	    clong(jj) = ltohx(jmax(jj))
489	continue
	if(verbose.eq.1) then
	    write (*,138) sumran,(opname(jj),clong(jj),jj=1,nfunc)
138	    format(/,'             total samples: ',i8,/,
     1		 'operation    max number of difference bits',/
     1		 (a16,a8)
     1	    )
	endif
	return
	end

	subroutine domath(ifunc,nran,power,thresh,ndiff,jmax)
c - perform math functions
c
c   Ifunc  is a number between 1 and 16 specifying which function to perform
c   Nran   is the number of times to perform the function
c   Power  is the approximate exponent of the operands
c   Thresh is the maximum bit difference that is allowed between
c	   the results computed by software versus those computed by the
c	   board.  If the bit difference is worse than this number,
c	   an error message will be printed
c   ndiff  returns the number of times that an operation resulted in
c	   a difference between the software result and the hardware result,
c	   without regard to the magnitude of the difference.
c   jmax   returns the bit difference for the worst error, without regard to
c	   the number of difference allowed

	include 'ffpusr.inc'
	real*4		op1,op2,op3,op4,ran,x,a1,a2
	integer*2	iop1(2),iop2(2),iop3(2),iop4(2),ifunc
	integer*4	thresh,jop3,jop4,jdiff,jmax,ndiff,nops,nran,power
	equivalence	(op1,iop1(1)),(op2,iop2(1))
	equivalence	(op3,iop3(1)),(op4,iop4(1))
	equivalence	(op3,jop3),(op4,jop4)

	ndiff = 0
	do 500 j = 1,nran
	a1 = (2.0 * ran(x) - 1.0) * power
	op1 = (ran(x) - 0.5) * 10.0 ** a1
	a2 = (2.0 * ran(x) - 1.0) * power
	op2 = (ran(x) - 0.5) * 10.0 ** a2
	op3 = 0
	op4 = 0
c
c - host code test
	if (ifunc.eq.1) then
	    op4 = op1 + op2
	    opcode = 1
	    nops = 2
	else if (ifunc.eq.2) then
	    opcode = 7
	    op4 = op1 - op2
	    nops = 2
	else if (ifunc.eq.3) then
	    opcode = 11
	    op4 = op1 * op2
	    nops = 2
	else if (ifunc.eq.4) then
	    opcode = 19
	    op4 = op1 / op2
	    nops = 2
	else if (ifunc.eq.5) then
	    opcode = 41
	    op4 = sin(op1)
	    nops = 1
	else if (ifunc.eq.6) then
	    opcode = 40
	    op4 = cos(op1)
	    nops = 1
	else if (ifunc.eq.7) then
	    opcode = 42
	    op4 = tan(op1)
	    nops = 1
	else if (ifunc.eq.8) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.9) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.10) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.11) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.12) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.13) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.14) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.15) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else if (ifunc.eq.16) then
	    opcode = 27
	    op4 = (op1)
	    nops = 0
	else
	    write(*,241) ifunc
241	format(/,'program err : bad ifunc value',i3)
	endif
c
c - send out the function code
	opcode=opcode+4096
	call outw (comreg,opcode)
	do 12 ii=1,nops
	if(ii.eq.1) then
	    call outw(dt1reg,iop1(1))
	    call outw(dt2reg,iop1(2))
	else
	    call outw(dt1reg,iop2(1))
	    call outw(dt2reg,iop2(2))
	endif
12	continue
c
c - check iordy and iodir bits in stcreg before reading data
	do 10 ii=1,100
	call inw (stcreg,iword)
	if(	(iandw(iword,iordy).eq.iordy).and.
     1		(iandw(iword,iodir).eq.iodir)     ) go to 20
10	continue
	cword(1) = wtohx(iword)
	write(*,115) cword(1)
115	format('  error:ffp function - stcreg=',a4)
	call exit(1)
20	continue
	call inw (dt1reg,iop3(1))
	call inw (dt2reg,iop3(2))
c
c - get difference
	do 11112 ii=1,100
	call inw (stcreg,iword)
	if(iandw(iword,idle).eq.idle) go to 22
11112	continue
	cword(1) = wtohx(iword)
	cword(2) = wtohx(opcode)
	write(*,11113) cword(1),cword(2)
11113	format('Status did not become idle - streg = ', a4, ' ifunct =  ',a4)
c -- It's not safe to continue on this kind of error, as the next access
c -- to the Sky board will probably time out.
	call exit(1)
22	continue
	delta = op3 -op4
	lxor  = ixorl(op3,op4)
	jdiff = iabs(jop3-jop4)

c -- If there is an error ...
	if(jop3.eq.0 .and. op4.lt.1.0e-37 .and. op4.gt.-1.0e-37) goto 499
	if(jdiff.ne.0) then
	    jmax = max0 (jmax,jdiff)
	    ndiff = ndiff + 1
	    if(jdiff.gt.thresh) then
		thresh=jdiff
	        clong(1) = ltohx(op1)
	        clong(2) = ltohx(op2)
	        clong(3) = ltohx(op3)
	        clong(4) = ltohx(op4)
	        clong(5) = ltohx(lxor)
	        clong(6) = ltohx(jdiff)
	        cword(1) = wtohx(opcode)
c -- Print name of function
		write(*,105) opname(ifunc)
105		format('       error: ',a16)
c -- Print operands and results
	        write(*,106) op4,op3,op1,cword(1),op2,clong(4),clong(3),
     1			     clong(1),cword(1),clong(2),j,clong(5),clong(6)
106		format(9x,'host',10x,'skyffp',10x,'operand #1',14x,
     1		'operand #2',/4x,2e14.6,' = ',e14.6,1x,'<opc=',a4,'>',e14.6,/,
     1		10x,a8,6x,a8,' = ',6x,a8,1x,'<opc=',a4,'>',1x,a8,/
     1		2x,'  i = ',i8,4x,'  xor = ',a8,'  bit difference = ',a8)
	    endif
	endif
499	continue
c -- end of "if there was an error"
500	continue
c -- end of loop over 1,nrand
139	return
	end
