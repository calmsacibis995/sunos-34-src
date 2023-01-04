	.data
	.asciz	"@(#)s2map.s 1.1 86/09/25"
	.even
	.text

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Debugger mapping functions on the Sun-2
 */

#include "../h/param.h"
#include "../sun2/asm_linkage.h"
#include "../sun2/mmu.h"
#include "../sun2/enable.h"

	.text
/*
 * Getpgmap() and Setpgmap() use software pte's which are
 * different than hardware pme's for sun2 machines.
 */
	ENTRY(Getpgmap)
	movl	sp@(4),d0		| get access address
	clrb	d0
	movl	d0,a0
	movc	sfc,d1			| save source function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,sfc			| set source function code
	movsl	a0@,d0			| read page map entry
	movc	d1,sfc			| restore source function code
	movl	d0,d1
	andl	#0xFC000FFF,d0
	andl	#0x00C00000,d1
	lsrl	#6,d1
	orl	d1,d0
	rts

	ENTRY(Setpgmap)
	movl	sp@(4),d0		| get access address
	clrb	d0
	movl	d0,a0
	movl	sp@(8),d0		| get page map entry to write
	movl	d0,d1
	andl	#0xFC00FFFF,d0		| throw away software fields
	andl	#0x30000,d1		| save type field
	lsll	#6,d1			| move it into place
	orl	d1,d0			| or it in
	btst	#27,d0			| user read access set?
	beq	3$
	bset	#25,d0			| yes, set user execute bit
3$:
	movc	dfc,d1			| save dest function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,dfc			| set destination function code
	movsl	d0,a0@			| write page map entry
	movc	d1,dfc			| restore dest function code
	rts				| done

	ENTRY(getsegmap)
	movl	sp@(4),d0		| get access address
	andl	#0xff8000,d0		| mask off bits
	addql	#SMAPOFF,d0		| set to segment map offset
	movl	d0,a0
	movc	sfc,d1			| save source function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,sfc			| set source function code
	moveq	#0,d0			| clear upper part of register
	movsb	a0@,d0			| read segment map entry
	movc	d1,sfc			| restore source function code
	rts				| done

	ENTRY(setsegmap)
	movl	sp@(4),d0		| get access address
	andl	#0xff8000,d0		| mask off bits
	addql	#SMAPOFF,d0		| set to segment map offset
	movl	d0,a0
	movl	sp@(8),d0		| get seg map entry to write
	movc	dfc,d1			| save dest function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,dfc			| set destination function code
	movsb	d0,a0@			| write segment map entry
	movc	d1,dfc			| restore dest function code
	rts				| done

	ENTRY(getusercontext)
	movc	sfc,d1			| save source function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,sfc			| set source function code
	movsb	USERCONTEXTOFF,d0	| move context reg into result
	andb	#CONTEXTMASK,d0		| clear high-order bits
	movc	d1,sfc			| restore source function code
	rts

	ENTRY(setusercontext)
	movb	sp@(7),d0		| get context value to set
	movc	dfc,d1			| save dest function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,dfc			| set destination function code
	movsb	d0,USERCONTEXTOFF	| move value into context register
	movc	d1,dfc			| restore dest function code
	rts

	ENTRY(getenable)
	movc	sfc,d1			| save source function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,sfc			| set source function code
	moveq	#0,d0			| clear upper part of result
	movsw	ENABLEREG,d0		| move enable reg into result
	movc	d1,sfc			| restore source function code
	rts

	ENTRY(setenable)
	movw	sp@(6),d0		| get context value to set
	movc	dfc,d1			| save dest function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,dfc			| set destination function code
	movsw	d0,ENABLEREG		| move value into context register
	movc	d1,dfc			| restore dest function code
	rts
