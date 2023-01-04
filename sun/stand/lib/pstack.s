|
|	@(#)pstack.s 1.1 9/25/86 Copyright Sun Micro
|
|
|
	.text
	.globl	_main
|
|	exception vectors for pdiag test
|
	.long	0x200000-4			| 0 initial SSP
	.long	prt			| 1 initial PC

	.long	buserr			| 2 buserror handler
	.long	addrerr			| 3 address error handler
	.long	exception		| 4 illegal instruction
	.long	exception		| 5 divide by zero
	.long	exception		| 6 CHK instruction
	.long	exception		| 7 TRAPV instruction
	.long	exception		| 8 privilege viola
	.long	exception		| 9 trace
	.long	exception		| a line 1010
	.long	exception		| b line 1111
	.long	exception		| c reserved
	.long	exception		| d reserved
	.long	exception		| e reserved
	.long	exception		| f uninit interrupt vector
	.long	exception		| 16-23  reserved
	.long	exception		| R reserved
	.long	exception		| R reserved
	.long	exception		| R reserved
	.long	exception		| R reserved
	.long	exception		| R reserved
	.long	exception		| R reserved
	.long	exception		| R reserved
	.long	exception		| spurious interrupt
	.long	exception		| level 1 autoint
	.long	exception		| level 2 autoint
	.long	exception		| level 3 autoint
	.long	exception		| level 4 autoint
	.long	exception		| level 5 autoint
	.long	exception		| level 6 autoint
	.long	exception		| level 7 autoint
	.long	exception		| trap 0 of 16
	.long	exception		| trap 1 of 16
	.long	exception		| trap 2 of 16
	.long	exception		| trap 3 of 16
	.long	exception		| trap 4 of 16
	.long	exception		| trap 5 of 16
	.long	exception		| trap 6 of 16
	.long	exception		| trap 7 of 16
	.long	exception		| trap 8 of 16
	.long	exception		| trap 9 of 16
	.long	exception		| trap a of 16
	.long	exception		| trap b of 16
	.long	exception		| trap c of 16
	.long	exception		| trap d of 16
	.long	exception		| trap e of 16
	.long	exception		| trap f of 16

. = 0x400				| end of vector table
	

	.data
	.asciz "@(#)pstack.s 1.1 9/25/86 Copyright Sun Micro"
	.even
uartinit:
	.word	2496		| this is the 9600 baud
	.word	1094		| setup from scctest.c
	.word	960
	.word	1506
	.word	2306
	.word	2901
	.word	3086
	.word	3328
	.word	3586
	.word	961
	.word	1514
	.word	3587
	.word	48
	.word	48
	.word	0xffff
	.even
digits:	.ascii	"0123456789ABCDEF"	| printing digits
	.even
	.text
|
|	CPU register defines
|
PAGEOFF		= 	0x0
SEGOFF		= 	0x5
CXREG		= 	0x6
SUPVCXREG	=	CXREG
USERCXREG	=	CXREG + 1
IDPROM		=	0x8
LEDREG		=	0xa
BERRREG		=	0xc
ENABLEREG	=	0xe
FC_UD		=	1		| user data accesses
FC_UP		=	2		| user program accesses
FC_MMU		=	3		| access to MMU stuff is in fc3
FC_SD		=	5		| supervisor data accesses
FC_SP		=	6		| supervisor program accesses
|
|	MMU related defines
|
MAXADDR		= 	0x1000000
NCONTEXTS	=	8
NSEGS		=	512
NPAGES		=	4096
SEGINCR		=	0x8000
PAGEINCR	=	0x800
MEMADDR		=	0x100000
MEMSIZE		=	0x100000
NMEMPAGES	=	MEMSIZE/PAGEINCR
MEMPAGEVALUE	=	0xfe000000
|
|	defines for devices
|
PRINTADDR	=	SEGINCR			| where is uart??
PRINTSEG	=	PRINTADDR + SEGOFF	| seg setup
PRINTPAGE	=	PRINTADDR + PAGEOFF	| page setup
PRINTSEGVALUE	=	0x1			| segment = second pmeg
PRINTPAGEVALUE	=	0xfe400004		| v rwxrwx P2 i/o scc's

PROMADDR	=	0			| where is prom??
PROMSEG		=	PROMADDR + SEGOFF	| seg setup
PROMPAGE	=	PROMADDR + PAGEOFF	| page setup
PROMSEGVALUE	=	0x0			| segment = second pmeg
PROMPAGEVALUE	=	0xfe400000		| v rwxrwx P2 i/o prom
PROMEND		=	SEGINCR			| one 32k seg mapped in

UARTACNTL	=	PRINTADDR + 4		| where is port A
UARTADATA	=	UARTACNTL + 2		| and port A data
UARTBCNTL	=	PRINTADDR + 0		| where is port B
UARTBDATA	=	UARTBCNTL + 2		| and port B data
|		status bits
RXREADY		=	0			| receive ready bit
TXREADY		=	2			| transmit ready bit
|
|	defines for buserror and enable registers
|
BERR_PARERRL	=	0x01
BERR_PARERRU	=	0x02
BERR_TIMEOUT	=	0x04
BERR_PROTERR	=	0x08
BERR_P1MASTER	=	0x10
BERR_VALID	=	0x80
BERR_WECARE	=	0x8f

EN_PARGEN	=	0x01
EN_INT1		=	0x02
EN_INT2		=	0x04
EN_INT3		=	0x08
EN_PARCHECK	=	0x10
EN_DVMA		=	0x20
EN_INT		=	0x40
EN_NORMAL	=	0x80
|
|	exceptions
|
	.data
buserrmess:
	.asciz	"\015\012Unexpected bus error!! %d7 @ %d6 berr(%d0)\007\007"
	.even
	.text
buserr:
	moveq	#~0x1b, d0		| display 1b for buserr
	movsb	d0, LEDREG
	moveq	#FC_MMU, d0
	movc	d0, sfc
	movsw	BERRREG, d0
	movl	sp@(2), d6		| get program counter
	movl	sp@(10), d7		| get fault address
	movl	#buserrmess, a4
	movl	#1$, a6
	jra	print
1$:	stop	#0x2700

	.data
addrerrmess:
	.asciz	"\015\012Unexpected address error!! %d7 @ %d6\007\007"
	.even
	.text

addrerr:
	moveq	#~0x1a, d0		| display 1a for address error
	movsb	d0, LEDREG
	movl	sp@(2), d6		| get program counter
	movl	sp@(10), d7		| get fault address
	movl	#addrerrmess, a4
	movl	#1$, a6
	jra	print
1$:	stop	#0x2700

	.data
exerrmess:
	.asciz	"\015\012Unexpected exception error!!#%d7 = %d6\007\007"
	.even
	.text

exception:
	moveq	#~0x1e, d0		| display 1e for exception
	movsb	d0, LEDREG
	movw	sp@(6), d6		| get vector offset and format
	movl	d6, d7			| set up vector #
	lsrl	#2, d7
	movl	#exerrmess, a4
	movl	#1$, a6
	jra	print
1$:	stop	#0x2700

print:	movl	d0, a3			| save d0 for later
	clrl	d3			| clear reg & digit counters
	clrl	d0
	movsw	d0, CXREG		| set up context zero plural
	movb	#PRINTSEGVALUE, d0	| map in uart for print
	movsb	d0, PRINTSEG		| map in uart for print
	movl	#PRINTPAGEVALUE, d0
	movsl	d0, PRINTPAGE

	movb	#PROMSEGVALUE, d0		| map in prom for data
	movsb	d0, PROMSEG
	movl	#PROMPAGEVALUE, d0
	movl	#PROMPAGE, a0
0$:	movsl	d0, a0@				| map in 32k of same
	addl	#PAGEINCR, a0
	cmpl	#PROMEND, a0
	jne	0$

1$:	tstb	a4@			| end of string yet??
	jne	990$			| yep, end this noise
	movl	a3, d0			| restore d0
	jmp	a6@			| and return
990$:	cmpb	#0x25, a4@		| is '%' ??
	jne	998$			| nope, print it
	clrl	d3			| else clear digit counter
	addql	#1, a4			| waste the %
	movb	a4@+, d0		| and find register to print
	cmpb	#0x64, d0		| is 'd'??
	jne	2$			| branch to see if 'a'
	movb	a4@+, d0		| get register #
	andl	#7, d0			| hope is '0'..'7'
	lsll	#2, d0			| make long pointer
	movl	#99$, a0		| get address of jumptable
	addl	d0, a0			| set up jump table address
	movl	a0@, a0			| get jump entry
	jmp	a0@			| do it

99$:	.long	100$			| jump table for register load
	.long	101$			| some of these will not be used
	.long	102$
	.long	103$
	.long	104$
	.long	105$
	.long	106$
	.long	107$

100$:	movl	a3, d2			| d0 was stored in a3
	jra	10$
101$:	movl	d1, d2			| usually the expected value
	jra	10$
102$:	movl	d2,d2			| just for consistency
	jra	10$
103$:	movl	d3,d2
	jra	10$
104$:	movl	d4,d2
	jra	10$
105$:	movl	d5,d2
	jra	10$
106$:	movl	d6,d2
	jra	10$
107$:	movl	d7,d2
	jra	10$

2$:	cmpb	#0x61, d0		| is 'a'??
	jne	998$			| nope, print char
	movb	a4@+, d0		| else get register #
	andl	#7, d0			| hope is '0'..'7'
	lsll	#2, d0
	movl	#199$, a0		| get address of jumptable
	addl	d0, a0			| set up jump table address
	movl	a0@, a0			| get entry
	jmp	a0@			| do it


199$:	.long	200$			| jump table for register load
	.long	201$			| some of these will not be used
	.long	202$
	.long	203$
	.long	204$
	.long	205$
	.long	206$
	.long	207$

200$:	movl	a0, d2
	jra	10$
201$:	movl	a1, d2
	jra	10$
202$:	movl	a2,d2
	jra	10$
203$:	movl	a3,d2
	jra	10$
204$:	movl	a4,d2
	jra	10$
205$:	movl	a5,d2
	jra	10$
206$:	movl	a6,d2
	jra	10$
207$:	movl	a7,d2

10$:	movl	d2, d0
	roll	#4, d0			| get next highest digit
	movl	d0, d2			| and save back
	andl	#0xf, d0		| mask it
	movl	d0, a0			| set up byte grab
	addl	#digits, a0
	movb	a0@, d0			| get char for this digit
11$:	btst	#TXREADY, UARTACNTL	| check TXREADY
	jeq	11$			| loop until ready
	movb	d0, UARTADATA		| then stuff out
	movl	#100, d0
12$:	dbra	d0, 12$			| delay a bit
	addql	#1, d3			| bump digit counter
	btst	#3, d3			| check digit count == 8
	jeq	10$			| it bit for 8*4 not set , go on
	jra	1$			| else go on printing
	
998$:	btst	#TXREADY, UARTACNTL	| check TXREADY
	jeq	998$			| loop until ready
	movb	a4@+, UARTADATA		| then stuff out
	movl	#100, d0
999$:	dbra	d0, 999$		| delay a bit
	jra	1$			| then go for next char
|
| start of the user test
|
prt:
	moveq	#FC_MMU, d0			| set up for MMU accesses
	movc	d0, sfc
	movc	d0, dfc

	moveq	#~0x01, d7			| start here
	movsb	d7, LEDREG
|
|
	clrl	d0				| run in context 0
	movsw	d0, CXREG			| set context
|
|	set segregs linear mapping
|
	lsll	#1,d7
	movsb	d7, LEDREG

	movl	#NSEGS-1, d6
	clrl	d0
	movl	#SEGOFF, a0			| set up all seg registers
0$:	movsb	d0, a0@				| stuff register
	addql	#1, d0				| bump pmeg # (wrap at 0xff)
	addl	#SEGINCR, a0			| bump seg pointer
	dbra	d6, 0$
|
|	now map in prom
|
	lsll	#1,d7
	movsb	d7, LEDREG

	movl	#PROMPAGEVALUE, d0
	movl	#PROMPAGE, a0
1$:	movsl	d0, a0@				| map in 32k of same
	addl	#PAGEINCR, a0
	cmpl	#PROMEND, a0
	jne	1$
|
|	map in uart
|
	lsll	#1,d7
	movsb	d7, LEDREG
	movl	#PRINTPAGEVALUE, d0
	movsl	d0, a0@(PAGEOFF)
|
|	inits uarts for later tests (uses uartinit above)
|	sets up A as I/O device, and sets up B same
|
	lsll	#1,d7
	movsb	d7, LEDREG

	movl	#uartinit, a0			| set up pram xfer for A
2$:	cmpb	#0xff, a0@			| are we done yet??
	jeq	5$
	movb	a0@+, UARTACNTL			| stuff
	moveq	#0x7f, d0			| and wait
3$:	dbra	d0, 3$
	jra	2$				| then do next byte

5$:	movl	#uartinit+2, a0			| set up pram xfer - resetworld
6$:	cmpb	#0xff, a0@			| are we done yet??
	jeq	8$
	movb	a0@+, UARTBCNTL			| stuff
	moveq	#0x7f, d0			| and wait
7$:	dbra	d0, 7$
	jra	6$				| then do next byte
|
|	map in memory
|
8$:	lsll	#1,d7
	movsb	d7, LEDREG

	movl	#NMEMPAGES-1, d6		| map in a meg at 0x100000
	movl	#MEMADDR, a0
	movl	#MEMPAGEVALUE, d0
9$:	movsl	d0 , a0@
	addql	#1, d0
	addl	#PAGEINCR, a0
	dbra	d6, 9$
	movl	#EN_PARGEN+EN_PARCHECK+EN_NORMAL, d0	| go to normal state
	movsw	d0, ENABLEREG
	movl	#MEMADDR, a0			| clear memory
	movl	#MEMSIZE, d0
	swap	d0
	lsrl	#2, d0				| make long/64k number
	subql	#1, d0				| make dbraable
	moveq	#-1, d1
10$:	movl	d0, a0@+			| write value
	dbra	d1, 10$
	dbra	d0, 10$
|
|	system stack is 0x200000 from reboot
|
	lsll	#1,d7
	movsb	d7, LEDREG
	movl	a7, a6				| set up user stack same??
	clrl	a6@				| set up stack entry
	clrl	a7@
11$:	jsr	_main				| call program
	movl	#0x81, d0
	movsb	d0, LEDREG
	jra	11$				| keep looping
