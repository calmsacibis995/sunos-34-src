LL0:
	.data
	.comm	_CGXBase,0x4
	.comm	_head,0x4
	.comm	_bds,0x228
	.even
	.globl	_bd_count
_bd_count:
	.long	0x0
	.comm	_bus_err,0x2
	.text
	.globl	_find_bds
_find_bds:
	link	a6,#0
	addl	#-LF46,sp
	tstb	sp@(-LP46)
	moveml	#LS46,sp@
	movb	#0x5c,a6@(-14)
	movb	#0xff,a6@(-15)
	clrl	_head
	movl	#_berr,0x8
	movl	#0x10,a6@(-4)
L51:
	cmpl	#0x80,a6@(-4)
	jge	L50
	movl	a6@(-4),d0
	moveq	#0xe,d1
	asll	d1,d0
	movl	d0,_CGXBase
	.data1
L53:
	.ascii	"Testing location 0x%x\12\0"
	.text
	movl	_CGXBase,sp@-
	movl	#L53,sp@-
	jbsr	_printf
	addql	#8,sp
	clrw	_bus_err
	movl	_CGXBase,a0
	movb	#0x33,a0@(0x1a00)
	tstw	_bus_err
	jne	L54
	movl	_CGXBase,d0
	addl	#0x2800,d0
	movl	d0,a6@(-8)
	movl	_CGXBase,a6@(-12)
	movl	a6@(-12),a0
	clrb	a0@
	movl	a6@(-8),a0
	movb	a6@(-14),a0@
	movl	a6@(-8),a0
	movb	a0@,a6@(-13)
	movl	a6@(-8),a0
	movb	a6@(-15),a0@
	movl	a6@(-8),a0
	movb	a0@,a6@(-13)
	movb	a6@(-13),d0
	movb	a6@(-14),d1
	notb	d1
	cmpb	d1,d0
	jne	L55
	tstw	_bus_err
	jne	L55
	.data1
L56:
	.ascii	"Device #%d found at address 0x%x.\12\0"
	.text
	movl	_CGXBase,sp@-
	movl	_head,a0
	movw	a0@(0x4),d0
	extl	d0
	movl	d0,sp@-
	movl	#L56,sp@-
	jbsr	_printf
	addl	#12,sp
	movl	_CGXBase,sp@-
	jbsr	_add_device
	addql	#4,sp
L55:
L54:
L49:
	addql	#0x1,a6@(-4)
	jra		L51
L50:
	tstl	_head
	jne	L58
	.data1
L59:
	.ascii	"WARNING\72 No Boards Detected in System. Use Manual mode.\12\0"
	.text
	movl	#L59,sp@-
	jbsr	_printf
	addql	#4,sp
L58:
	jra	LE46
LE46:
	moveml	a6@(-LF46),#LS46
	unlk	a6
	rts
   LF46 = 64
	LS46 = 0x0
	LP46 =	20
	.data
   .text 
   .globl  _berr
_berr:
   moveml  #0xc0c0,sp@-
   jsr     __berr
   moveml  sp@+,#0x0303
   rte
	.text
	.globl	__berr
__berr:
	link	a6,#0
	addl	#-LF61,sp
	tstb	sp@(-LP61)
	moveml	#LS61,sp@
	.data1
L63:
	.ascii	"Bus Error.\12\0"
	.text
	movl	#L63,sp@-
	jbsr	_printf
	addql	#4,sp
	movw	#0x1,_bus_err
	addl	#0xA,sp
	addql	#2,sp@
	subql	#2,sp
	jra	LE61
LE61:
	moveml	a6@(-LF61),#LS61
	unlk	a6
	rts
   LF61 = 0
	LS61 = 0x0
	LP61 =	12
	.data
	.text
	.globl	_add_device
_add_device:
	link	a6,#0
	addl	#-LF64,sp
	tstb	sp@(-LP64)
	moveml	#LS64,sp@
	.data1
L66:
	.ascii	"Enter add_device.\12\0"
	.text
	movl	#L66,sp@-
	jbsr	_printf
	addql	#4,sp
	tstl	_head
	jne	L67
	movl	#0x2e,sp@-
	movl	_bd_count,sp@-
	jbsr	lmul
	addql	#8,sp
	addl	#_bds,d0
	movl	d0,_head
	jra		L68
L67:
	movl	#0x2e,sp@-
	movl	_bd_count,d0
	subql	#0x1,d0
	movl	d0,sp@-
	jbsr	lmul
	addql	#8,sp
	addl	#_bds,d0
	movl	d0,a6@(-4)
	movl	#0x2e,sp@-
	movl	_bd_count,sp@-
	jbsr	lmul
	addql	#8,sp
	addl	#_bds,d0
	movl	a6@(-4),a0
	movl	d0,a0@(0x2a)
L68:
	movl	#0x2e,sp@-
	movl	_bd_count,d0
	addql	#0x1,_bd_count
	movl	d0,sp@-
	jbsr	lmul
	addql	#8,sp
	addl	#_bds,d0
	movl	d0,a6@(-4)
	movl	a6@(-4),a0
	movl	a6@(8),a0@
	movl	a6@(-4),a0
	movw	_bd_count+0x2,a0@(0x4)
	movl	a6@(-4),a0
	clrl	a0@(0x2a)
	.data1
L69:
	.ascii	"Return from add_device.\12\0"
	.text
	movl	#L69,sp@-
	jbsr	_printf
	addql	#4,sp
	jra	LE64
LE64:
	moveml	a6@(-LF64),#LS64
	unlk	a6
	rts
   LF64 = 4
	LS64 = 0x0
	LP64 =	16
	.data
