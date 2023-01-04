| 	stubs.s 	1.1	86/09/25

	.globl	_nullsys, _xxboot, _xxprobe, _ttboot
_nullsys:
_xxprobe:
_xxboot:
_ttboot:
	movl	#-1,d0
	rts
