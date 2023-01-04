SEGINCR		=	0x8000
PRINTADDR	=	SEGINCR			| where is uart??

UARTACNTL	=	PRINTADDR + 4		| where is port A
UARTADATA	=	UARTACNTL + 2		| and port A data
UARTBCNTL	=	PRINTADDR + 0		| where is port B
UARTBDATA	=	UARTBCNTL + 2		| and port B data
|		status bits
RXREADY		=	0			| receive ready bit
TXREADY		=	2			| transmit ready bit

	.data
	.asciz	"@(#)psaio.s 1.1 9/25/86 Copyright Sun Micro"
	.even
	.text

_getchar:
	btst	#RXREADY, UARTACNTL		| wait for char
	jne	_getchar
	clrl	d0				| clear other bits
	movb	UARTADATA, d0			| get char
	rts					| return to innocence

_maygetchar:
	btst	#RXREADY, UARTACNTL		| wait for char
	jeq	1$				| if not there, return -1
	clrl	d0				| clear other bits
	movb	UARTADATA, d0			| get char
	rts					| return to innocence
1$:	moveq	#-1, d0				| return -1 if no char
	rts

_putchar:
	btst	#TXREADY, UARTACNTL		| check for ready
	jne	_putchar			| wait until
	movb	sp@(7), UARTADATA		| then stuff out
	rts
