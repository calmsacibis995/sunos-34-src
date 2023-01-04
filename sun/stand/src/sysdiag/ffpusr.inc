c---ffpusr.inc
c
c @(#)ffpusr.inc 1.1 9/25/86 Sun Microsystems Inc.
c
	common /globlc/	rgname,name,null,icode,ucdlin,fname,opname
     1	,cword,clong
	character*1	null,icode,ucdlin(8)
	character*4	cword(8)
	character*6	rgname(6),name
	character*8	clong(8)
	character*12	fname
	character*16	opname(16)
	common /globlw/	rgport,iword,jword,kword,opcode,rgdata,
     1	reset,intenb,ihalt,runenb,single,sngrun,iordy,idle,iodir,iscr
     1	,cmbuff,iold,isleep,pass
	integer*2	rgport(6),iword,jword,kword,opcode,rgdata(6)
	integer*2	reset,intenb,ihalt,runenb,single,sngrun
	integer*2	iordy,idle,iodir,iscr,cmbuff(8),iold,pass,isleep
	common /globll/	ucode, jerr , dtbuff , verbose, loadtest
	integer*4	ucode, jerr , dtbuff(8,8) , verbose, loadtest
	common /asciic/	ascii
	character*1	ascii(16)
	common /asciiw/	ascmap
	integer*2	ascmap(16)
	integer*2	comreg,stcreg,dt1reg,dt2reg,mc1reg,mc2reg
	integer*2	iandw
	character*4	wtohx
	character*8	ltohx
	integer*2	hxtow
	integer*4	hxtol
	equivalence	(rgport(1),comreg),(rgport(2),stcreg)
	equivalence	(rgport(3),dt1reg),(rgport(4),dt2reg)
	equivalence	(rgport(5),mc1reg),(rgport(6),mc2reg)
1000	format(a)
c---end of ffpusr.inc
