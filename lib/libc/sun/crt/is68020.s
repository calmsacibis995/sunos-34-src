|
| @(#)is68020.s	1.1 (Sun) 9/24/86
| is68020():
|	returns 1 if running on 68020, 0 if running on 68000 or 68010
|	The trick is that the old processors ignore scaled indexing
|
	.globl	_is68020
_is68020:
	moveq	#1,d0
|	lea	a6@(0,d0:l:4),a1	| scale an index of 1 by a factor of 4
	.long	0x43f60c00		| this atrocity is to fool the assembler
	lea	a6@(4),a0		| should get same answer by adding 4
	cmpl	a0,a1			| if answers are same, we're on 68020
	beq	1f
	moveq	#0,d0			| otherwise 68000 or 68010
1:
	rts
