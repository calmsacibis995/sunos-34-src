
|
|	@(#)desdiag.s 1.1 86/09/27
|	Copyright (c) 1985 by Sun Microsystems, Inc.
|	Power-Up Diagnostics for the am8068 Data Ciphering Processor (DES chip)
|
|	This diagnostic routine attempts to return information about
|	which parts of the machine are broken, as bitmaps, so they can
|	be configured out of the system and it can continue to run.
|
|	Global Register allocation:
|	d0 = Scratch reg, typically the data read out for comparision
|	d1 = 
|	d2 = Contains bitmap for bad contexts and summaries -- 1=valid.
|	d3 = 
|	d4 = 
|	d5 = 
|	d6 = Contains ancillary information
|	d7 = Contains the primary (eg expected) data value for the test
|	a0*= Points to error message for early exits from diagnostics
|	a1*= Address in use as of last error detected in loops
|	a2*= Data written   as of last error detected in loops
|	a3*= Data read back as of last error detected in loops
|	a4 = 
|	a5 = 
|	a6 = Scratch address reg used for auto-inc thru byte strings
|	a7 = Scratch address reg (can't be used for byte auto-inc tho)
|
|	  *= This information is valid upon exit to diagret.
|

#include "../sun2/assym.h"
#include "../h/led.h"

DESADDR	= 0x2000	| Address where we map DES chip in

DESSETREG = DESADDR+DESSELOFF	| Write addr of reg we want to touch
DESREG	  = DESADDR+DESREGOFF	| Touch (read or write) register

	.globl	_desdiag
	.text

_nochip:
	bclr	#DESFOUND,d2		| Indicate no chip and no failure
	jra	_diagret

Error:
	bclr	#DESBIT,d2		| Clear DES-OK bit
	jra	_diagret		| Just jump back to main monitor code

|
|	Start of Data Ciphering Processor diagnostics
|
_desdiag:
| FIXME
	movl	a0,d0			| An error already found?
	jne	_diagret		| Yes, just exit, forget DES.
| FIXME
	movw	#FC_MAP,a7		| Be sure we can access the maps.
	movc	a7,sfc
	movc	a7,dfc

	moveq	#256+~L_DES,d0		| Tell the world we're running.
| The 256+ foils the damn assembler's refusal to put a byte in moveq
| that doesn't sign-extend into the same longword.
	movsb	d0,LEDOFF

	movl	#PME_DES,a7
	movsl	a7,DESADDR+PMAPOFF	| Map in the DES to location DESADDR

	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
	movb	#DESC_RESET,DESREG	| Reset it.
	moveq	#100,d0
desh1:	dbra	d0,desh1		| Delay awhile for reset
	movb	#DESR_MODE,DESSETREG	| We want mode reg now
	movb	DESREG,d0		| Read mode register
	andb	#0x0F,d0		| Just keep the bits it sets on reset
	cmpb	#DESM_MC_SE,d0		| Is it "right"?
	jeq	desok0			| (yes)
	cmpb	#0x0F,d0		| Is it "nonexistent"?
| FIXME, this doesnt work -- bus is floating, we get garbage.  Still true?
	jeq	_nochip			| Yes, just quit this diagnostic.
	lea	deserr0,a0
	movl	d0,a3			| Save bad value
	jra	Error

deserr0:.ascii	"DES Reset\0"
	.even

desok0:
	movb	#DESM_M_ONLY+DESM_ENCRYPT+DESM_ECB,DESREG | Write our mode
	moveq	#100,d0
desh2:	dbra	d0,desh2		| Delay awhile for mode reg write
	movb	DESREG,d0		| Read it back
	andb	#0x1F,d0		| Mask out only 5 bits supplied
	cmpb	#DESM_M_ONLY+DESM_ENCRYPT+DESM_ECB,d0 | Is it right?
	jeq	desok1			| (yes)
	lea	deserr1,a0
	movl	d0,a3			| Save bad value
	jra	Error

deserr1:.ascii	"DES mode\0"
	.even

| Attempt to load a clear encryption key
desok1:
	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
	movb	#DESC_LOAD_E_KEY,DESREG	| Set command
	nop				| Extra delay for write to mode reg
	moveq	#0,d6			| About to load 8 bytes

desl1:
	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
	movb	DESREG,d0		| Read back status
	movb	#DESR_IO,DESSETREG	| We'll want input/output reg later
	movb	#DESS_CMD_PEND,d1	| Get bit mask
	andb	d0,d1			| Is command pending?
	jne	desok3			| Yes, load byte
	lea	deserr3,a0
	movl	d6,a1			| Save iteration number
	movl	d0,a3			| Save bad status value
	jra	Error

deserr3:.ascii	"DES CP1\0"
	.even

|
|	Test Data
|
OurEKey:
	.byte	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01

desok3:
	movb	pc@(OurEKey-2-.,d6:w),DESREG	| Write a byte to input reg
	addql	#1,d6			| Go to next byte
	cmpw	#8,d6			| Are we done?
	jne	desl1

	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
	movb	DESREG,d0
	movb	#DESS_CMD_PEND,d1	| Get bit mask
	andb	d0,d1			| Command oughta be done.
	jeq	desok4
	lea	deserr4,a0
	movl	d0,a3			| Save bad status value
	jra	Error

deserr4:.ascii	"DES CP0\0"
	.even

desok4:
	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
	movb	#DESC_START_ENC,DESREG	| Start encryption, please
	nop				| Extra delay for write to mode reg
	moveq	#0,d6			| About to load 8 bytes

desl2:
	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
	movb	DESREG,d0		| Read back status
	movb	#DESS_MST_FLAG,d1	| Get bit mask
	andb	d0,d1			| Is command pending?
	jne	desok5			| Yes, load byte
	lea	deserr5,a0
	movl	d0,a3			| Save bad status
	movl	d6,a1			| Save iteration count
	jra	Error

deserr5:.ascii	"DES MF1\0"
	.even

|
|	Test Data
|
OurClear:
	.byte	0x95,0xF8,0xA5,0xE5,0xDD,0x31,0xD9,0x00

desok5:
	movb	#DESR_IO,DESSETREG	| We want input/output reg
	movb	pc@(OurClear-2-.,d6:w),DESREG	| Write a byte to input reg
	addql	#1,d6			| Go to next byte
	cmpw	#8,d6			| Are we done?
	jne	desl2

|	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
|	movb	DESREG,d0
|	movb	#DESS_MST_FLAG,d1	| Command oughta be done.
|	andb	d0,d1			| Is command pending?
|	jeq	desok6
|	lea	deserr6,a0
|	movl	d0,a3			| Save bad status
|	jra	Error
|
|deserr6:.ascii	"DES MF0\0"
|	.even
|
|desok6:
	moveq	#100,d6			| Count down looking for output

desl6:
	movb	#DESR_CMD_STAT,DESSETREG	| We want cmd/status reg
	movb	DESREG,d0		| Get status to check for output
	movb	#DESS_SLAVE_FLAG,d1	| Get bit mask
	andb	d0,d1			| Is command pending?
	dbne	d6,desl6		| Not yet, loop awhile
	jne	desok7			| Got it, get the data

	lea	deserr7,a0
	movl	d0,a3			| Save bad status after 100th iter
	jra	Error

deserr7:.ascii	"DES SF1\0"
	.even

desok7:
	moveq	#0,d6
	jra	desl7

|
|	Test Data
|
OurCipher:
	.byte	0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00

desl7:
	movb	pc@(OurCipher-2-.,d6:w),d7	| Expected data
	movb	#DESR_IO,DESSETREG	| We want input/output reg
	movb	DESREG,d0		| Get output data
	cmpb	d0,d7
	jne	dese8
	addql	#1,d6			| Go to next byte
	cmpw	#8,d6			| Are we done?
	jne	desl7
	jra	desok8
dese8:	lea	deserr8,a0
	movl	d6,a1			| Save iteration number
	movl	d7,a2			| Save "should be" data
	movl	d0,a3			| Save "read from chip" data
	jra	Error

deserr8:.ascii	"DES Data\0"
	.even

desok8:
|	We should stop the DES here and check that it stops.  Wotthehell.
	jra	_diagret		| End of test.

