|
|	@(#)ex.s 1.1 9/25/86 Copyright Sun Micro";
|
|	ex_handler is what a C routine sets ex_vector->buserr (or any other
|	exception) to be when exception is desired as the handler.
|	exception_print controls printing (0 no 1 yes)
|	exception_handler is a (*int)() that is used if non zero as the
|	address of a user routine to deal with this exception.  A
|	(struct ex_info) is passed to the routine when called.
	.globl	_exception_print, _exception, _ex_handler, _exception_handler
	.data
regsave:
	.long	0,0,0,0		| save area for d0,d1,a0,a1
_exception_print:
	.long	1
_exception_handler:
	.long	0

	.text
_ex_handler:
	moveml	#0x0303, regsave	| save <d0,d1,a0,a1>
	jsr	_exception		| call super chicken
	moveml	regsave, #0x0303	| bring out your dead
	rte
