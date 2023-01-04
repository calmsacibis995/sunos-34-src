|       @(#)diag.s 1.7 86/11/03
|       Copyright (c) 1986 by Sun Microsystems, Inc.
|	Power-Up Diagnostics for Sun-3 Processor Board
|
|	Revision History
|
|	Who	Date	 Revision
|	MSP	8/23/85	 1.0	Initial Release for Carrera Boot PROM
|	MSP	9/8/85	 1.1	Update for M25 Selftest
|	MSP	10/25/85 1.2	To agree with PROM revision level
|	MSP	10/31/85 1.3    To fix 1.2 MMU segment map RAM setup
|	MSP	11/22/84 1.4	Add Sirius selftest code
|
|	This module is executed immediately after a power-up reset or a "K2"
|	reset command. The execution time of the selftest is a function of the 
|	workstation type and its available memory.  Exeuction times are provided 
|as|	as follows:
|
|	workstation	memory size	execution time
|	-----------	-----------	--------------
|
|	Carrera		2 Megabytes	xx
|			4 		xx
|			8		xx
|		       16		xx
|		       24		xx
|
|	It performs a building block sequence of tests which initially assume
|	only the CPU chip, EPROM, and addressing and bus paths between them
|	to work.
|
|	For normal system booting the diagnostic switch at the rear edge of
|	the CPU card should be in the off position.  During normal system 
|	booting, all selftest tests are executed except extended memory test.
|	If a selftest error is detected, the selftest program will go into a 
|	scope loop of the failing test, displaying its test number in the 
|	diagnostic LEDs at the rear edge of the CPU card.  The most significant
|	LED (test error) will be on with the least significant 5 LEDs indicating
|	the test number.

|	To troubleshoot the failure:
|
|	(1) If a RS_232 CRT/terminal is available, connect it to serial port A
|	at the rear edge of the CPU board and set its characteristics as follows:
|
|		BAUD rate:	9600 (Port A)
|		BAUD rate:      1200 (Port B)
|		data bits:	8
|		stop bits:	1
|		parity:		none	
|
|	(2) Place the diagnostic switch in the "diagnostic boot" position, and
|	re-power the system to restart selftest.  The selftest program should 
|	output its program name and being testing.  At the beginning of each
|	test the test name will be output to the terminal CRT.
|	During the tests the position of the "diagnostic mode" switch at the 
|	rear edge of the CPU board will cause the following actions to be taken:
|
|		(1) At top of each test the test name will be output to serial
|		port A.
|
|		(2) Upon error the error message with appropriate good and bad
|		status will be output to serial port A.  After printing the
|		error message, the program will enter a scope loop suitable
|		for troubleshooting the failure, this will be indicated by 
|		the most significant LED being turned on to indicated a test
|		failure and that the failing test is being looped.
|
|		(3) To proceed to the next test if additional failure information
|		is desired, press the keyboard space bar to proceed to the next
|		failing pattern or test.  To restart the selftest press the "b"
|		character to "begin" the test again.
|
|
|	CPU register use during the tests is as follows:
|
|	        d0 - scratch, generally observed value for print
|       	d1 - scratch, generally expected value for print
|       	d2 - scratch
|       	d3 - scratch
|       	d4 - scratch
|       	d5 - scratch
|       	d6 - destroyed in print routine
|       	d7 - test control bits, test number
|
|       	a0 - scratch (saves d0 in print routine)
|       	a1 - scratch (saves d1 in print routine)
|       	a2 - scratch (saves d2 in print routine)
|       	a3 - scratch (saves d3 in print routine)
|       	a4 - print routine message pointer
|       	a5 - RAM test address
|       	a6 - PC return address for all routines
|       	a7 - top of test loop saved PC
|       	USP - 
|
|	Test control register d7 bit assignemnts:
|
|	bit	assignment
|	15	setup bus error svc for top of test loop if 1
|	14	don't print messages 
|	13	perform burn in of selftest
|	7	test error flag: 0 if error
|	6	exception class error: 0 if error
|	5	not used
|	4-0	test number: inverse of test number
|

#include        "../sun3/assym.h"
#include        "../h/led.h"
 
|----------------------------------------------------------------------
| CPU Function Code defines
|

FC_UD           =       1               | user data accesses
FC_UP           =       2               | user program accesses
FC_MMU          =       3               | access to MMU stuff is in fc3
FC_SD           =       5               | supervisor data accesses
FC_SP           =       6               | supervisor program accesses

|--------------------------------------------------------------------
| MMU Control Space defines
|

IDPROM          =       0x00000000      | system identification prom
PAGEOFF         =       0x10000000      | page map
SEGOFF          =       0x20000000      | segment map
CXREG           =       0x30000000      | contect register
ENABLEREG       =       0x40000000      | system enable register
USERENREG       =       0x50000000      | user enable register
BERRREG         =       0x60000000      | bus error register
LEDREG          =       0x70000000      | diagnostic register
CACHETOFF       =	0x80000000      | cache data 
CACHEDOFF       =	0x90000000      | cache tags 
NCACHETGS	=	4096		| number of cache tag words
CACHETINCR	=	0x10		| cache tag address increment
DIRSCC          =       0xf0000000      | MMU bypass to SCC for diags

|------------------------------------------------------------------------
| MMU Parameter defines
|

MAXADDR		=       0x10000000	 
MAXSEG          =       0x10000000
MAXPAGE         =       0x20000
NCONTEXTS       =       8               | number of contexts
NSEGS           =       2048            | number of segments per context
NPAGES          =       4096            | number of pages
BYTES_PER_PG	=	8192		| bytes per page
SEGINCR         =       0x20000         | segment map increment
PAGEINCR        =       0x2000          | page map increment
NCACHEWDS       =	16384           | number of cache data words
CACHEDINCR      =	4               | cache data address increment
MEMINCR         =       0x4             | size of long
MEMADDR         =       0x00000000      | start of memory for memory tests
MEMSIZE         =       0x02000000      | size of memory
MEMPAGE         =       0xf0000000
INVALIDPAGE     =       0x70000000      | invalid page
PROTECTPAGE     =       0x90000000      | protected page
WRITEBIT        =       0x40000000      | write allow bit in page map
SUPERBIT        =       0x20000000      | supervisor bit in page map
BUSFPADDR       =       0xe0000000
ACCESSED        =       0x02000000      | accessed bit
MODIFIED        =       0x01000000      | modified bit
VALID           =       0x80000000      | valid bit
CACHETMASK	=	0xcfff3700	| cache tags wr/rd mask

|-----------------------------------------------------------------------
|       defines for Type 1 space devices
|
KYBDM_PAGE       =       0xf4000000      | physical page, kybd/mouse
SCC_PAGE         =       0xf4000010      | physical page, SCC 
EEPROM_PAGE      =       0xf4000020      | physical page, EEPROM
TODCLK_PAGE      =       0xf4000030      | physical page, TOD clock
MEM_ERR_PAGE     =       0xf4000040      | physical page, parity register
INT_PAGE         =       0xf4000050      | physical page, interrpt register 
ETHER_PAGE       =       0xf4000060      | physical page, ethernet chip 
EPROM_PAGE       =       0xf4000080      | physical page, EPROM
ECC_MEM_PAGE	 =	 0xf40000f0	 | physical page, ECC MEM control


|--------------------------------------------------------------------------
| Virtual Pages assigned to Type 1 I/O
|

SCC_BASE	=	0xFFFE000	| virtual page, SCC
INT_BASE	=	0xFFF2000	| virtual page, interrupt ena reg
MERR_BASE	=	0xFFF4000	| virtual page, parity err reg
MERR_ADDR	=	0xFFF4004	| virtual page, memory err reg
EEPROM_BASE     =       0xFFF6000       | virtual page, for EEPROM
EEPROM_MEM_SZ	=	0xFFF6015	| memory to test
ECC_MEM_BASE	=	0xFFF8000	| virtual page, ECC control regs
ECC_MEM_ENA_REG =	ECC_MEM_BASE 	| virtual address, ECC Mem enable
ECC_SYNDROME_REG =	ECC_MEM_BASE + 4 | virutal address, ECC syndrome reg
ECC_DIAG_REG	=	ECC_MEM_BASE + 8 | virtual address, ECC diag reg
#ifdef SIRIUS
MEMINIT_PAGE	=	0xFFFA000	| page used to intialize upper memory
#endif SIRIUS
CLK_BASE 	=	0xFFFC000	| the clock base address virtual

|-------------------------------------------------------------------------
| Bus Error Register defines
|
BERR_FPA        =       0x04            | FPA not there error
BERR_P2         =       0x08            | FPA operation error
BERR_P1MASTER   =       0x10            | VME bus error
BERR_TIMEOUT    =       0x20            | timeout error
BERR_PROTERR    =       0x40            | protection error
BERR_VALID      =       0x80            | invalid page


|------------------------------------------------------------------------
| System Enable Register defines
|

EN_DIAG         =       0x01            | read back diagnostic switch
EN_FPA          =       0x02            | enable floating point accelerator
EN_COPY         =       0x04            | enable copy mode to video memory
EN_VIDEO        =       0x08            | enable video display & copy mode
EN_CACHE        =       0x10            | enable external cache
EN_DVMA         =       0x20            | enable system DVMA
EN_FPP          =       0x40            | enable floating point processor
EN_NORMAL       =       0x80            | enable boot state for buserror register

|------------------------------------------------------------------------
| Interrupt Enable Register defines
|

EN_INT          =       0x01            | enable all interrupts
EN_INT1         =       0x02            | software interrupt level 1
EN_INT2         =       0x04            | software interrupt level 2
EN_INT3         =       0x08            | software interrupt level 3
EN_INT4         =       0x10            | enable video interrupt level 4
EN_INT5         =       0x20            | enable clock interrupt level 5
EN_INT6         =       0x40            | reserved
EN_INT7         =       0x80            | enable clock interrupt level 7

|----------------------------------------------------------------------
| Parity Register defines
|

PARERR0         =       0x01            | parity error on bits 0 to 7
PARERR8         =       0x02            | parity error on bits 8 to 15
PARERR16        =       0x04            | parity error on bits 16 to 23
PARERR24        =       0x08            | parity error on bits 24 to 31
PARCHECK        =       0x10            | enable parity checking on read
PARTEST         =       0x20            | write parity with opposite parity
EN_PARINT       =       0x40            | enable level 7 interrupt on error
PARINT          =       0x80            | parity interrupt is pending
|-----------------------------------------------------------------------
| Ecc register defines (cpu brd)
CE_ERR		=	0x01		| mask for CE error
UE_ERR		=	0x02		| mask for UE error
EN_ECC		=	0x10		| enable CE reporting
EN_CEINT	=	0x40            | enable level 7 interrupt on error

|----------------------------------------------------------------------------
| SCC (Serial Communications Controller) defines
|
| For Model 25 SCC access for test status and error reporting must go thru
| the MMU; while all other Sun-3 models use the MMU bypass.

#ifdef M25
UARTACNTL	=	SCC_BASE + 4	| SCC port A control address(M25)
UARTBCNTL       =       SCC_BASE    	| SCC port B control address(M25)
#else
UARTACNTL	=	DIRSCC + 4	| SCC port A control address 
UARTBCNTL       =       DIRSCC      	| SCC port B control address
#endif M25
UARTADATA	=	UARTACNTL + 2	| SCC port A data address
UARTBDATA	=	UARTBCNTL + 2	| SCC port B data address
RXREADY		=	0		| SCC receiver ready bit
TXREADY		=	2		| SCC transmitter ready bit


|----------------------------------------------------------------------------
| Page map defines
|

PME_RD_ONLY_1	=	0x80000001	| ready only, page 1
PME_INVALID_1	=	0x00000001	| invalid, page 1
PME_MEMORY_0	=	0xF0000000	| rd/wr access, page 0
PME_MEMORY_1	=	0xF0000001	| rd/wr access, page 0

|------------------------------------------------------------------------------
| Test Control Option bits
|


op_restart	=	0x73	
op_next		=	0x20	
op_burn_in	=	0x62	
op_no_print	=	0x6E	
op_print_all	=	0x70
op_skip_end	=	0x1B

|------------------------------------------------------------------------------
| Test Control Bit defines
|
|	Define bits of register d7 which provide test control/status info.

bit_set_bus_err	=	15		| set bus err svc to top of loop
bit_no_print	=	14		| no print flag
bit_fail	=	13		| selftest pass/fail flag
bit_print_all	=	12		| no loop/print all errors
bit_burn_in	=	11		| selftest burn in flag
bit_no_read	=	10		| no read char flag
bit_echo_mode   =       9               | diagnostic echo mode flag
bit_port_flag	=	8		| print$ SCC Port A/B flag
bit_tst_err	=	7		| current test fail flag
bit_exc_err	=	6		| exception class fail flag	

|------------------------------------------------------------------------------
| Miscellaneous defines
|
|MEM_size	=	0x1FFFA000	| save memory size
led_delay	=	0xc350		| delay count for LED viewing
DIAGSW		=	0x0		| diagnostic switch bit
Test_patt	=	0x5A972C5A	| initial test pattern
patt_end	=	0x972C5A5A	| ending pattern
NXM_address     =       BYTES_PER_PG    | nonexistent memory address
level_1_vector	=	0x64		| level 1 autovector 
level_5_vector	=	0x74
level_7_vector	=	0x7c		| the same as nmi vector
nmi_vector	=	0x7c		| nmi (parity error) vector
low_mem_addr	=	0x1000		| low memory address for testing
MONBSS_BASE_LOW	=	0x0FFFE000	|
sr_index	=	0x0		| stack index, sr
pc_index	=	0x2		| stack index, pc
vector_index	=	0x6		| stack index, vector (vector only)
access_index	=	0x10		| stack index, access addr(bus err only)
delay_10_sec    =       0x0FFFFF        | 10 second delay
SYNMSK		=	0xff000000	| mask for syndrome code
EADDR           =       0x7ffffe        | mask for ecc address
MEG32		=	0x20
MEG31		=	0x1f
MEG8		=	0x800000	| 8 meg bytes -128k value
MEG1		=	0x1		| used in mem test (sirius)
ECCDIAG2	=	0x1000		| value to enable ecc chips into diag mod2
SYNDROME	=	0x800		| mem loc to save syndrome code
NOBD		=	SYNDROME+4	| address for no. of mem bds in sys
sv_mem_sz	=	NOBD+4		| save memory size loc
mem_addr	=	sv_mem_sz+4	| save starting address
| more storage here because we run out of reg's in mem-test (sirius only)
save_d4		=	mem_addr+4
save_d5		=	save_d4+4
save_d6		=	save_d5+4
save_a0		=	save_d6+4
| for Sirius only!!!
| if true, then loop on error else if error == CE go on
ue_error	=	save_a0+4
ce_error	=	ue_error+4
MEM_size        =       ce_error+4       | save memory size
|------------------------------------------------------------------------------
EPROM_sz	=	0x10000		| EPROM size
clk_int_hsec	=	0x02		| bit pos for TOD 1/100 int
clk_setup	=	0x1c		| cmd to run the TOD int's
|			0x10 = start int's
|			0x08 = start running
|			0x04 = run in 24 hr mode
|			0x00 = use 32 khz base freq.
clk_cmdreg	=	0x11		| THE clk int reg. offset


  	.globl	_selftest, _diag_berr
	.text

|------------------------------------------------------------------------------
|	Diagnostic tests start here
|

_selftest:
	movl	#0x77770000,d7		| 7777 pattern for "in diag.s" 
	movw	#0x2700, sr		| put cpu on level 7

	subl	a7,a7
	movc	a7, vbr			| reset the vector base register

	moveq	#FC_MMU,d0
	movc	d0,sfc
	movc	d0,dfc
					| with valid,write enable set



|-------------------------------------------------------------------------------
|	Float a 1 (on) bit thru the LEDs at rear edge of CPU bd to demonstrate
|	they work.

	movw	#0x7FFF,d0		| start with no bits lit
LED_loop:
	movsb	d0,LEDREG		| ***output pattern to LEDs***
	
	movl	#led_delay,d1		| delay count for human viewing

10$:	dbra	d1,10$			| view delay loop

	rolw	#1,d0			| shift bit left 
	cmpb	#0xFF,d0		| loop til 0-bit is shifted out
	bmi	LED_loop

#ifdef M25
	
|------------------------------------------------------------------------------
|	SCC (Z8530) Wr/Rd Test Thru MMU (M25 only)
|
|	Writes and reads SCC Port A with a float one byte pattern.
|
|	For model 25 selftest this is the first executed test which sets up 
|	virtual address 0xFFFE000 to point to the physical base address of 
| 	the SCC chip used for Serial port A and B.  This means Segment Map address
| 	0xFF contains 0xFF and Page Map address 0xFFF contains 0xf4000010,
|	the physical page address of the SCC chip.  The MMU write setup and the 
|	write/read of SCC WR12/RR12 is all contained in the scope loop to permit
|	scoping and troubleshooting.
|
|	Test type:	FATAL if error, loops forever.
|	LED display:	0x80 upon error; 0x00 if no error.

Test_00:
	bset	#bit_no_print,d7	| disable printouts for this test
	bset	#bit_no_read,d7		| disable reading chars
	movb	#~0,d7			| test # is 0
	lea	10$,a6			| save PC return
	jra	test$			| display test # in LEDs
10$:
	movl	#SEGOFF+MAXSEG-SEGINCR,a0 | 2FFE0000 >> a0
	movl	#PAGEOFF+MAXSEG-SEGINCR+MAXPAGE-PAGEINCR,a1 | 1FFFE000 >> a1
	moveq	#1,d1			| initial write pattern = 1
14$:
	lea	20$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
20$:
	moveq	#FC_MMU,d0
	movc	d0,sfc			| setup source and dest fc
	movc	d0,dfc
	clrl	d0
	movsb	d0,CXREG		| 0 >> CXREG
	movb	#0xFF,d0
	movsb	d0,a0@			| ***0xFF >> Seg address 0x7FF***	
	movl	#SCC_PAGE,d0
	movsl	d0,a1@			| ***0xF4000010 >> Page address 0xFFF
	movb	#0x0c,UARTACNTL		| ***select SCC WR 12***
	movw	#0x100,d4		| chip recovery time delay
30$:	dbra	d4,30$
	movb	d1,UARTACNTL		| ***write SCC WR 12***
	movw	#0x100,d4		| chip recovery time delay
40$:	dbra	d4,40$
	movb	#0x0c,UARTACNTL		| ***select SCC RR 12***
	movw	#0x100,d4		| chip recovery time delay
50$:	dbra	d4,50$
	movb	UARTACNTL,d0		| ***read SCC RR 12***
	cmpb	d0,d1			| write byte = read byte?
	beq	60$			| br if equal
	lea	60$,a6			| save PC return
	jra  	error$				| Error!! SCC write/read
					| data compare error thru
					| MMU path!
60$:
	lea	70$,a6			| save PC return
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
70$:
	aslb	#1,d1			| next byte pattern
	bne	14$			| if not last pattern
	bclr	#bit_no_print,d7	| reenable print to SCC port A	
	bclr	#bit_no_read,d7		| renable char reads from port A

#endif M25

	movsb   ENABLEOFF,d0            | read diag switch
        btst    #DIAGSW,d0              | diag sw = 0?
        bne     90$                     | if 0, skip printout

	bset	#bit_no_print,d7
90$:
| Setup SCC chip for 1200 Baud, no parity, 8 data bits, 1 stop bit

	lea	100$,a6			| save PC return
	jra	UARTinit		| init the MMU bypass SCC port A
100$:					| for non-model 25 workstations

| Output Program name to serial port A.

	lea	program_name,a4
	lea	110$,a6
	jra	print$			| "Sun_3 Selftest"
110$:
Start:
|------------------------------------------------------------------------------
| PROM Checksum Test
|
|	Calculate a checksum for all addresses of the PROM except the last
|	and compare with the last, which should contain the checksum.
|
|	Test type:	FATAL if error, loops forever
|	LED display:	0x81 upon error; 0x01 if no error

Test_01:
	movb	#~1,d7			| test #
	lea	Test_01_txt,a4		| test descriptor text
	lea	10$,a6			| save PC return
	jra	test$			| display test text and number
10$:
	lea	20$,a6			| save PC return
	jra	loop$			| <<<TOP OF TEST LOOP>>>
20$:
	subl	a5,a5			| start at PROM/virtual addr 0
	moveq	#FC_SP,d0		| select instruction fetch(PROM) space
	movc	d0,sfc			| for movs FC space access
	clrl	d0
40$:					 
	movsb	a5@+,d1			| ***read a PROM byte***
	andl	#0xFF,d1		| insure only 8 bits 
	addl	d1,d0			| accumulate checksum
	cmpl    #EPROM_sz - 2,a5        | at last 2 bytes?
	bne	40$			| if not

	movsw	a5@+,d1			| get expected PROM checksum
	andl    #0xFFFF,d0              | strip off don't cares
	moveq	#FC_MMU,d3
	movc	d3,sfc			| restore sfc to MMU=default
	cmpw	d0,d1			| is stored checksum = calcualated?
44$:
 	beq	50$			| if yes
	lea	chksm_err_txt,a4	
	lea	50$,a6
	jra	error$			| error! checksum in last 2 bytes of 
50$:					| EPROM not = accumulated checksum!!
	lea	60$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
60$:

#ifndef	M25
|-----------------------------------------------------------------------------
| User DVMA Enable Register Test
|
|	Data from 0xFF to 0x00 is written to the DVMA register and then read
|	back and compared.
|
|	Test type: 	FATAL if error, loops forever
|	LED display: 	0x82 upon error; 0x02 if no error
|

Test_02:
	movb	#~2,d7			| test #
	lea	Test_02_txt,a4		| test descriptor text
	lea	10$,a6			| save PC return
	jra	test$			| display test text and number
10$:
	movl	#USERENREG,a5		| address of User DVMA Enable Reg
	movl	#0xFF,d1		| initial pattern = 0xFF
14$:
	lea	20$,a6			| save return PC
	jra	loop$			| <<<TOP OF TEST LOOP>>>
20$:
	movsb	d1,a5@			| ***write User Ena Register***
	movsb	a5@,d0			| ***read User Ena Register***
	andb	#0xff,d0
	movl	d0,d2
	eorb	d1,d2			| form xor of good vs bad
	beq	30$			| if OK
	lea	wr_rd_err_txt,a4		| error text msg address
	lea	30$,a6			| save return PC
	jra	error$			| error! User Ena Register write
30$:					| data not = read data!
	lea	40$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
40$:
	dbra	d1,14$			| last pattern?

#endif	M25	
|----------------------------------------------------------------------------
| Context Register Test
|
|	Data from 0x07 to 0x00 is written to the Context Register and then 
| 	read back and compared.
|
|	Test type: 	FATAL if error, loops forever
|	LED display:	0x83 upon error; 0x03 if no error
|
Test_03:
 	movb	#~3,d7			| test #
	lea	Test_03_txt,a4		| test descriptor text
	lea	10$,a6			| save PC return
	jra	test$
10$:
	moveq	#NCONTEXTS-1,d1		| initial pattern = 7
14$:
	lea	20$,a6			| save return PC
	jra	loop$			| <<<TOP OF TEST LOOP>>>
20$:
	clrl    d0                      | clear d0 for read data
	movsb	d1,CXREG			| ***write Context Register*** 
	movsb	CXREG,d0			| ***read Context Register***
	andb    #NCONTEXTS-1,d0		| strip unused bits
	movl	d0,d2
	eorb	d1,d2			| for xor for good vs bad
	beq	30$			| if OK 
	clrl	d3
	movsb	d3,CXREG		| 0 > CXREG for print$ routine
	lea	wr_rd_err_txt,a4	| error text msg address
	lea	30$,a6			| save return PC
	jra	error$			| error! Context Register write
30$:					| data not = read data!
	lea	40$,a6
	clrl	d3
	movsb	d3,CXREG	| CXREG must be 0 for loop$end
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
40$:
	dbra	d1,14$			| decrement pattern, last?
|----------------------------------------------------------------------------
| Segment Map Write/Read Test
|
| Write and read each address of the MMU Segment RAM with a float one
| data pattern. Upon error loop on failing Segment RAM address.
| For M25 selftest, preserve address 0xFF for terminal I/O.
|
|	Test type:	FATAL if error, loops forever
|	LED display:	0x84 upon error; 0x04 if no error

Test_04:
	movb	#~4,d7			| test #
	lea	Test_04_txt,a4		| test descriptor text
	lea	10$,a6			| save PC return
	jra	test$
10$:
	moveq	#NCONTEXTS - 1,d4	| number of contexts 
20$:
	movl	#SEGOFF,a5		| Segment Map RAM base address
40$:
	movl	#NSEGS-2,d5		| number of segments/context,preserve 0xFF	
45$:
	moveq	#1,d1			| initial write pattern
50$:
	lea	60$,a6			| save pc return
	jra	loop$			| <<<TOP OF TEST LOOP>>>
60$:
	movsb	d4,CXREG		| ***load CX register***
	clrl	d0			| clear register for read data
	movsb	d1,a5@			| ***write segment RAM address***
	movsb	a5@,d0			| ***read back segment RAM address***
	andl	#0xFF,d0		| strip off other read bits
	movl	d0,d2			| copy read data to d0 for xor
	eorb	d1,d2			| xor of write and read data
	beq	70$			| if write = read data
	clrl	d3
	movsb	d3,CXREG		| 0 > CXREG for print$ routine
	lea	xor_rd_err_txt,a4	| error text address
	lea	70$,a6			| save pc return
	jra	error$			| ERROR! DATA COMPARE ERROR IN
70$:					| WRITING AND READING MMU SEGMENT
	lea	80$,a6			| save pc return
	movsb	d3,CXREG		| 0 > CXREG for loop$end
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
80$:
	aslb	#1,d1			| next write pattern
	bne	50$			| if not last pattern
	addl	#SEGINCR,a5		| next Segment RAM address
	dbra	d5,45$			| if not RAM address 0 yet
	dbra	d4,20$			| next context #
	clrl	d3
	movsb	d3,CXREG		| 0 > CXREG for print$ routine

|----------------------------------------------------------------------------
| Segment Map Test
|
|	Performs three passes of write/read tests.
|
|		1st pass: writes 5A,2D,96 to consecutive Segment Map addresses
|		2nd pass: writes 2D,96,5A ....
|		3rd pass: writes 96,5A,2D ....
|
|	Test type: 	FATAL if error, loops forever
|	LED display:	0x84 upon error; 0x04 if no error
|
Test_05:
	movb	#~5,d7			| test #
	lea	Test_05_txt,a4		| test descriptor text
	lea	10$,a6			| save PC return
	jra	test$
10$:
	movl	#Test_patt,d2		| point to 1st test pattern
6$:
	lea     11$,a6                  | save PC return
        jra     loop$                    | <<<TOP OF TEST LOOP>>>
11$:
	moveq   #7,d4                   | 1st context = 7
	moveq   #1,d3                   | modulo 3 pattern count
12$:
	movsb	d4,CXREG		| load Context Register
	movl	#SEGOFF,a5		| Segment Map RAM base address
	movl	#NSEGS-2,d5 		| number of segments/context -1
					| Seg Addr 0xFF must be preserved
					| for Serial port A terminal I/O!!
14$:
	rorl	#8,d1
	subql	#1,d3
	bne	15$
	movl	d2,d1			| reinit modulo 3 pattern generator
	moveq   #3,d3                   | modulo 3 pattern count
15$:
	movsb   d1,a5@                  | ***write Segment Map address***
	addl    #SEGINCR,a5             | next Segment Map address
	dbra	d5,14$			| last segment address in context?
	dbra	d4,12$			| next context

	moveq	#1,d3			| modulo 3 pattern count
	moveq	#7,d4			| 1st context = 7
18$:
	movsb	d4,CXREG		| load Context Register
	movl	#SEGOFF,a5		| Segment Map RAM base address
	movl	#NSEGS-2,d5 		| number of segments/context
24$:
	rorl	#8,d1
	subql	#1,d3
	bne	36$
	movl	d2,d1
	moveq	#3,d3			| reinit pattern generator
36$:
	clrl    d0
	movsb	d4,CXREG		| load Context Register
        movsb   a5@,d0                  | ***read Segment Map address***
        cmpb	d0,d1			| write pattern = read pattern?
	beq     38$
	movl	d1, d6
        andl    #0xFF,d1
	rorl	#8,d3	| make a "0" to clr CXREG for M25 print$
	movsb	d3,CXREG	| CXREG must be 0 for M25 print$
	roll	#8,d3
        lea     mem_rd_err_txt,a4
        lea     37$,a6
        jra     error$
37$:
	movl	d6, d1
38$:
	addl    #SEGINCR,a5
	dbra	d5,24$			| decrement segment addr count
	dbra	d4,18$			| next context
					| byte pattern written!
	lea	40$,a6
	rorl	#8,d3		| make a "0" to clr CXREG for M25 loop$end
	movsb	d3,CXREG	| clr CXREG for loop$end
	roll	#8,d3
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
40$:
	rorl	#8,d2			| next modulo 3 pattern set
	cmpl	#patt_end,d2		| last pattern set?
	bne	6$			| if not
	bra	Test_06
	clrl	d0
	movsb	d0,CXREG	| clr CXREG for M25 print$
|-----------------------------------------------------------------------------
| Page Map Test
|
| 	The page map test runs in context 0 only, with the segment map set
| 	to map virtual address =  physical address in a one-to-one fashion:
|		virtual address 0 = physical address 0
|		  "       "     1 =   "        "     1
|		  "       "     n =   "        "     n
|	Three passes of write/read tests are performed on the Segment Map RAM
|
|	1st pass	2nd pass	3rd pass
|	--------	--------	--------
|	A5972C5A	5AA5972C	2C5AA597
|	5AA5972C	2C5AA597	A5972C5A
|	2C5AA597	972C5AA5	5AA5972C

Test_06:
	movb	#~6,d7			| test #
	lea	Test_06_txt,a4		| test descriptor text
	lea	6$,a6			| save PC return
	jra	test$
6$:
	movsb	d0,CXREG		| set context to 0
	movl	#NSEGS-1,d6		| # of Segment Map addresses cnt
	movl	#SEGOFF+MAXADDR-SEGINCR,a5 | top Segment Map address
8$:
	movsb	d6,a5@			| map virtual = physical for Context 0
	subl	#SEGINCR,a5		| next Segment Map address
	dbra	d6,8$	
10$:
	movl	#Test_patt,d2
11$:
	lea	12$,a6			| save PC return
	jra	loop$			| <<<TOP OF TEST LOOP>>>
12$:
	movl	#NPAGES-1,d4		| # of Page Map addresses
	movl	#PAGEOFF,a5		| Segment Map RAM base address
14$:
	movl	d2,d3			| init pattern generator
	movl	#3,d6			| modulo 3 pattern count
16$:
	movsl	d3,a5@			| ***write Page Map address***
	subql	#1,d4			| decrement page # cnt
	ble	18$			| if done
	addl	#PAGEINCR,a5		| next Page Map address
	rorl	#8,d3
	subql	#1,d6
	bne	16$
	bra	14$
18$:
	movl	#PAGEOFF,a5		| Segment Map RAM base address
	movl	#NPAGES-1,d4 		| number of pages to do 
20$:
	movl	d2,d3			| initial pattern generator
	movl	#3,d6			| modulo 3 pattern count
24$:
	movsl	a5@,d0			| ***read Page Map address***
	movl	d3,d1
#ifdef	M25
	andl	#0xFF0007FF,d1		| strip unused data bits
	andl	#0xFF0007FF,d0
#else
	andl    #0xFF07FFFF,d1          | strip unused data bits
	andl    #0xFF07FFFF,d0
#endif M25
	cmpl	d0,d1			| write pattern = read?
	beq	34$
	lea	mem_rd_err_txt,a4
	lea	34$,a6
	jra	error$			| error! Segment read byte not
34$:					| byte pattern written!
	addl	#PAGEINCR,a5		| next Page Map address
	rorl	#8,d3			| next pattern
	subql	#1,d4			| last RAM address?
	ble	40$
	subql	#1,d6			| decrement modulo 3 pattern count
	bne	24$
	bra	20$
40$:
	lea	50$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
50$:
	rorl	#8,d2			| next modulo 3 pattern set
	cmpl	#patt_end,d2		| last pattern?
	bne	11$			| if not

|-------------------------------------------------------------------------------
| Setup trap/vector service for tests that follow which test MMU bus error traps
| soft interrupts, parity generator/checker, and nmi interrupt from parity
| check error.  First unity map page map RAM since segment map RAM was left
| unity mapped from Test 05, the Page Map Test.

setup_traps:
|
| First, unity map memory for all memory space.
|

	movw	#0xFFF,d5
	movl    #PME_MEMORY_0,d0           | First page map entry
	lea	PAGEOFF,a5		| initialize at to pt to lowest page
5$:
	movsl   d0,a5@			| Write page map entry
	addql   #1,d0                   | Bump page number in map entry
        addl    #BYTES_PER_PG,a5        | Bump page address
	dbra	d5,5$ 
6$:

| Second, setup last 3 virtual pages to point to SCC, Interrupt Register,
| and Parity Register.

	
	movl	#PAGEOFF,a5
	lea	SCC_PAGE,a0
	movl	#SCC_BASE,d0	
	movsl	a0,a5@(0,d0:L)		| page for SCC
	lea	INT_PAGE,a0
	movl	#INT_BASE,d0
	movsl	a0,a5@(0,d0:L)		| page for Interrupt Enable Register
	lea	MEM_ERR_PAGE,a0
	movl	#MERR_BASE,d0
	movsl	a0,a5@(0,d0:L)		| page for Memory error registers
	lea     ECC_MEM_PAGE,a0         | ECC Memory Control Registers
        movl    #ECC_MEM_BASE,d0
        movsl   a0,a5@(0,d0:L)
					| map in Clock chip for TOD test
	lea	TODCLK_PAGE,a0
	movl	#CLK_BASE,d0
	movsl	a0,a5@(0,d0:L)
#ifdef M25
	lea     EEPROM_PAGE,a0
        movl    #EEPROM_BASE,d0
        movsl   a0,a5@(0,d0:L)
#endif M25
#ifdef	SIRIUS
|       Enable Memory module 0 by writing 0x40 to it

        movw    #0x40,d0
        lea     ECC_MEM_ENA_REG,a5
        movw    d0,a5@                  | write ECC MEM ENABLE REGISTER
#endif	SIRIUS

| Third, setup stack pointer and unexpected trap/vector service

	movl	#0x800,sp		| set stack pointer 
	lea	unex_bus_err,a0
	movl	a0,0xc			| setup address error svc
	lea	0x10,a5			| fill 0x10 to 0x3FC with
	lea	unex_trap_err,a0	| unexpected trap error svc
40$:
	movl	a0,a5@+
	cmpw	#0x400,a5		| at 0x400 yet?
	bne	40$

|------------------------------------------------------------------------------
| CPU-to-Memory Path Test
|
| This test writes each address of the block of addresses from 0x400 to
| 0x1000 with float one then float zero data patterns.  The test loops on
| the failing address in a tight scope loop.  It is the first write/read
| test using the CPU-to-Memory Data Bus Path.

Test_07:
	movb	#~7,d7			| test #
	lea	Test_07_txt,a4		| test descriptor text
	lea	6$,a6			| save PC return
	jra	test$
6$:
	lea	0x400,a5		| starting memory address 
	moveq	#1,d1			| initialize write pattern
10$:
	lea	20$,a6			| save pc return
	jra	loop$			| <<<TOP OF TEST LOOP>>>
20$:
	clrl	d0			| clear read data register
	movl	d1,a5@+			| ***write memory address***
	notl	d1			| complement data pattern
	movl	d1,a5@			| ***discharge data bus on nxt addr***
	notl	d1			| recomplement data pattern
	movl	a5@-,d0			| ***read memory address***
	movl	d1,d2			| copy to d2 for xor
	eorl	d0,d2			| write data = read data?
	beq	30$			| if yes
	lea	xor_rd_err_txt,a4	| error msg test address
	lea	30$,a6			| save pc return
	jra	error$			| ERROR! WRITE/READ DATA COMPARE
30$:					| ERROR WHILE WRITING AND READING
					| ADDRESSES 0x400 to 0x1000.	
	lea	50$,a6			| save pc return
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
50$:
	asll	#1,d1			| next float bit data pattern
	bne	10$			| if not last pattern
        addl    #4,a5                   | inc address 
        cmpl    #0x1000,a5              | last address been tested
        bne     10$                     | no, continue testing 

|------------------------------------------------------------------------------
| Nonexistant Memory Bus Error Test
|
| Attempt to read a nonexistant memory address and verify a bus error occurs.
|
 	
Test_08:
	movb	#~8,d7			| 7 > LED display
	lea	Test_08_txt,a4
	lea	10$,a6
	jra	test$
10$:
	lea	14$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
14$:
	lea	0x800,sp		| init stack ptr
	lea	20$,a0
	movl	a0,8			| setup bus error vector to point
					| to this test
#ifdef M25
	movl    #0xC00007FF,d0          | set rd valid,write allowed for page 1
#else
	movl    #0xC0004000,d0          | set rd valid,write allowed for page 1
#endif M25
        lea     BYTES_PER_PG,a5         | virtual page 1 address
        movl    #PAGEOFF,d1
        movsl   d0,a5@(0,d1:L)

	movw	NXM_address,d2		| ***try to read nonexistant address***
	nop
	nop
	nop
	nop
	lea	NXM_err_txt,a4
	lea	20$,a6
	jra	error$			| trying to read a nonexistant memory
					| address should cause a bus error!!
20$:
        movl    #0x800,sp               | reinit stack pointer 
30$:
	movl	#BERR_TIMEOUT,d1	| setup expeced bus error contents
	movsb	BERRREG,d0		| ***read bus error register***
	andb	#0xFC,d0		| strip off unused status bits
	cmpb	d1,d0			| bus error regr contents as expected?
	beq	34$			
	eorl	d1,d2
	lea	be_status_txt,a4
	lea	34$,a6
	jra	error$			| bus error reg contents not as expected
34$:					| after a timeout bus error!!
	lea	40$,a6
	jra	loop$end
40$:
	lea	unex_bus_err,a0		| restore unexpected bus error service
	movl	a0, 8	
|-----------------------------------------------------------------------------
| Interrupt Test
|
| Force a level 1 soft interrupt to verify an autovector level 1 interrupt will
| occur.

Test_09:
	movb	#~9,d7			| 8 > LED display
	lea	Test_09_txt,a4
	lea	10$,a6
	jra	test$			| display test # and name
10$:
	lea	20$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
20$:
	lea	0x800,sp		| init  stack ptr
	lea	level_1_vector,a0	| setup vector address for level 1 auto
	lea	40$,a1
	movl	a1,a0@			| store vector at 0x64 address
	movw	#0x2000,sr		| put CPU on level 0 
	nop
	nop	
	nop
	movb	#EN_INT+EN_INT1,INT_BASE | enable level 1 autovector interrupt
	movw	#0xFFFF,d4
30$:
	dbra	d4,30$
	lea	int_error_txt,a4
	lea	40$,a6
	jra	error$			| Enabling software interrupt level 1
					| interrupt shld cause a level 1 auto-
					| vector, but didn't!!
40$:
	movw	#0x2700,sr
	movb	#0,INT_BASE		| clear interrupt enable bits
	lea	50$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
50$:
|-----------------------------------------------------------------------------
|       <TOD Clock Interrupt Test>
|
| Enable the TOD Clock (Intersil 7170) to interrupt at a 100 Hz rate
| CPU interrupt enable register and level 5.
| This test does not verify the time delay to be accurate which is left
| for later, "C" code tests.

Test_0A:
        movb    #~0x0A,d7                  | test # > LED display
        lea     Test_0A_txt,a4
        lea     10$,a6
        jra     test$                   | display test # and name

10$:
	lea	20$,a6
	jra	loop$
20$:
        lea     0x800,sp                | init stack pointer
        lea     level_5_vector,a0       | setup vector address
        lea     40$,a1
        movl    a1,a0@                  | store vector at vector address
        movw    #0x2000,sr              | put CPU on level 0

        movb    #0,INT_BASE             | ***disable all interrupts first***
        movb    #0,CLK_BASE+clk_intrreg | ***disable TOD interrupts***
        movb    CLK_BASE+clk_intrreg,d0 | ***clear any pending interrupts***
        movb    #clk_int_hsec,CLK_BASE+clk_intrreg | enable TOD (100 Hz) int's
        nop
        nop
        movb    CLK_BASE+clk_intrreg,d0 | ***clear any pending interrupts***

        movb    #EN_INT5+EN_INT,INT_BASE | enable level 5
        movb    CLK_BASE+clk_intrreg,d0 | ***clear any pending interrupts***
	movb	#clk_setup,CLK_BASE+clk_cmdreg | allow TOD int's
        movl    #0xFFFFFF,d0		| delay to wait for interrupt
30$:
        subql   #1,d0
        bne    30$ 

        lea     int_TOD_error_txt,a4
        lea     40$,a6
        jra     error$
                                        | ERROR!  ASSERTING A TOD CLOCK AUTO-
                                        | VECTOR SHLD CAUSE A LEVEL 7 INT
                                        | BUT DIDN'T!!
40$:
        movw    #0x2700,sr
        movb    #0xC,CLK_BASE+clk_cmdreg  | disable clock int's 
        movb    #0,INT_BASE             | clear interrupt enable bits
        lea     50$,a6
        jra     loop$end                | bottom of test loop
50$:
        lea     unex_trap_err,a1       | restore level 7 vector
        lea     level_5_vector,a0
        movl    a1,a0@
60$:

|-----------------------------------------------------------------------------
| MMU Control/Status Tests
|

Test_0B:
	movb	#~0xB,d7		| 0xA > LED display
	lea	MMU_access_txt,a4
	lea	10$,a6
	jra	test$			| display test name


|-----------------------------------------------------------------------------
| Verify read accessing a page sets the access status bit for that page
|

10$:
	lea	20$,a6	
	jra	loop$			| <<<TOP OF TEST LOOP>>>
20$:
	movl	#PME_MEMORY_1,d3	| set rd valid,write allowed for page 1
	lea	BYTES_PER_PG,a5		| virtual page 1 address
	movl	#PAGEOFF,d4
	movsl	d3,a5@(0,d4:L)		| set wr/rd access bits
	movb	a5@,d2			| ***read from page 0 address***
	movsl	a5@(0,d4:L),d0		| ***read page status**
	movl	#PME_MEMORY_1+ACCESSED,d1 | exepected MMU page status
	movl	d0,d5
	eorl	d1,d5			| form xor of good vs bad
	beq	30$			| if good 
	lea	access_err_txt,a4
	lea	30$,a6
	jra	error$			| read accessing a page shld set its
					| accessed status bit!!
30$:
	lea	Test_0B0,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>

|-----------------------------------------------------------------------------
| Verify write accessing a page sets the access and modified bits for that page
|

Test_0B0:
	movb	#~0xB,d7		| test # for LED display
	lea	MMU_accmod_txt,a4
	lea	110$,a6
	jra	test$			| display test #
110$:
	lea	120$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
120$:
	movl	#PME_MEMORY_1,d3	| set rd valid,write allowed for page 1
	lea	BYTES_PER_PG,a5		| virtual page 1 address
	movl	#PAGEOFF,d4
	movsl	d3,a5@(0,d4:L)		| clear page stat bits, virtual pg 1
	movb	d2,a5@			| ***write to page 1 address***
	movsl	a5@(0,d4:L),d0		| ***read page status**
	movl	#PME_MEMORY_1+ACCESSED+MODIFIED,d1 | exepected MMU page status
	movl	d0,d5
	eorl	d1,d5
	beq	130$			| if so
	lea	access_err_txt,a4
	lea	130$,a6
	jra	error$			| read accessing a page shld set its
					| accessed status bit!!
130$:
	lea	Test_0B1,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>

|-----------------------------------------------------------------------------
| Invalid Read Access Violation Test
|
| Verify that attempting to read access an invalid page causes a bus error 
| with invalid status bit set in the bus error register.

Test_0B1:
	movb	#~0xB,d7		| test # >> LEDs
        lea     MMU_invalid_txt,a4
	lea	200$,a6
	jra	test$			| display test #
200$:
	lea	250$,a0
	movl	a0,8			| setup bus error trap svc for test
	lea	220$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
220$:
	movl	#PME_INVALID_1,d3	| set rd valid,write allowed for page 1
	lea	BYTES_PER_PG,a5		| virtual page 1 address
	movl	#PAGEOFF,d4
	movsl	d3,a5@(0,d4:L)		| clear page stat bits, virtual pg 1
	movb	a5@,d2			| ***attempt to read page address***

	lea	inv_ac_err_txt,a4
	lea	260$,a6
	jra	error$			| Read accessing an invalid page shld
					| cause a bus error trap!!
250$:
        movl    #0x800,sp               | reinit stack pointer 
	movl	#BERR_VALID,d1
	clrl	d0
	movsb	BERRREG,d0		| ***read bus error register***
	andb	#0xFC,d0		| strip off don't care bits
	movl	d0,d2
	eorl	d1,d2			| xor for good vs bad
	beq	280$
	lea	be_status_txt,a4
	lea	280$,a6
	jra	error$			| Error! Bus error register status
					| not correct after read accessing an
260$:					| invalid page!!
280$:
	lea	Test_0B2,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>


|-----------------------------------------------------------------------
| Verify that attempting to write a write protected page causes a bus error
| with the protect error bit set in the bus error register.

Test_0B2:
	movb	#~0xB,d7			| test # >> LEDs
        lea     MMU_protect_txt,a4
	lea	300$,a6
	jra	test$
300$:
	lea	350$,a0
	movl	a0,8			| setup bus error trap svc for test
	lea	320$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
320$:
	movl	#PME_RD_ONLY_1,d3	| set rd valid,write allowed for page 1
	lea	BYTES_PER_PG,a5		| virtual page 1 address
	movl	#PAGEOFF,d4
	movsl	d3,a5@(0,d4:L)		| clear page stat bits, virtual pg 1
	movb	d2,a5@			| ***write to page 1 address***
	lea	wr_prot_err_txt,a4
	lea	360$,a6
	jra	error$			| Write accessing a write protected page
					| shld cause a bus error trap!!
	bra	360$
350$:
	movl	#BERR_PROTERR,d1
	clrl	d0
	movsb	BERRREG,d0		| ***read bus error register***
	andb	#0xFC,d0		| strip off don't care bits
	movl	d0,d2
	eorb	d1,d2			| status as expected?
	beq	380$
	lea	be_status_txt,a4
	lea	380$,a6
	jra	error$			| Error! Bus error register status
					| not correct after read accessing an
360$:					| invalid page!!
380$:
	lea	390$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
|
|	Restore wr/rd access to page 1
390$:
	movl	#PME_MEMORY_1,d3
	lea	BYTES_PER_PG,a5
	movl	#PAGEOFF,d4
	movsl	d3,a5@(0,d4:L)

#ifndef	SIRIUS
|-----------------------------------------------------------------------------
| Parity Error Tests
|

Test_0E:
	movb	#~0xE,d7		| 0xD > LED display
	lea	Test_0C_txt,a4
	lea	4$,a6
	jra	test$			| display test name and number

|-----------------------------------------------------------------------------
| Verify writing and reading a known good memory address with parity
| checking enabled does not cause a parity error.

4$:
	lea	50$,a0
	movl	a0,nmi_vector		| setup parity err trap svc for test

	moveq	#1,d3			| initialize write pattern
	lea	0x1000,a5		| address to write/read data 
6$:
	lea	10$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
10$:
	lea	0x800,sp		| init stack ptr
	movb	#EN_PARINT,d1		| expected parity error status
	movb	#0,MERR_ADDR		| ***rd memory error addr reg to clr***
	nop
	movb	#EN_INT,INT_BASE	| ***enable all interrupts***
	nop
	movw	#0x2000,sr		| ***enable CPU interrupts***
	nop	
	movb	#EN_PARINT,MERR_BASE	| ***enable parity interrupt***
	nop
	movl	d3,a5@			| ***write float pattern***
	clrl	d2
	movl	a5@,d2			| ***read back***
	clrl	d0
	nop
	nop
	movb	MERR_BASE,d0		| ***read parity error register***
	andb	#0xf0,d0		| ignore parity byte bits
	cmpb	d0,d1			| status as expected?
	beq	60$			| if no compare error
	lea	par_st_err_txt,a4	
	lea	60$,a6
	jra	error$			| error!! nominal reading memory with
					| good parity generation shld not
	bra	60$			| cause bad parity error status!!
50$:
        movl    #0x800,sp               | reinit stack pointer
	clrl	d0
	movb	MERR_BASE,d0		| ***read parity error register***
	lea	unex_par_int,a4
	lea	60$,a6
	jra	error$			| error!!  reading memory has caused
					| a parity error trap!!
60$:
	movb	#0,MERR_ADDR		| *** write memory error reg to clr***
	lea 	70$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
70$:
	asll	#1,d3			| next float one pattern
	bne	6$			| last pattern?
|------------------------------------------------------------------------------
| Verify that forcing a parity error by writing bad parity shld cause
| a parity error nmi trap (autovector level 7) with correct parity error
| register status.

Test_0F:
	movb	#~0xF,d7
	lea	100$,a6
	jra	test$			| test # >> LEDs
100$:
	lea	150$,a0
	movl	a0,nmi_vector		| setup nmi (parity err) vector for test
	lea	0x1000,a5		| mem address to wr/rd
	moveq	#0x8,d1		| expected parity err byte status bit
104$:
	moveq	#1,d3			| initialize write pattern
	lea	110$,a6			| 
	jra	loop$			| <<<TOP OF TEST LOOP>>>
110$:
	lea	0x800,sp		| init stack ptr
	movb	#0,MERR_BASE		| ***clear parity err register***\
	movb	#0,MERR_ADDR		| ***write mem. err reg to clr***
	movb	#PARTEST,MERR_BASE	| ***enable inverse parity generation***
	movb	d3,a5@			| ***write bad parity in byte address***
	movb	#PARCHECK+EN_PARINT,MERR_BASE | enable parity err interrupt***
	movb	a5@,d2			| ***read bad parity to cause nmi trap***
	clrl	d0
	movb	MERR_BASE,d0		| ***read parity err register***
	
	nop
	nop
	lea	no_par_trap_txt,a4
	lea	170$,a6
	jra	error$			| error!! Reading bad parity shld cause
					| a parity error trap but didn't!!
150$:
	orb	#PARCHECK+EN_PARINT+PARINT,d1 | expected parity error status
	movb	MERR_BASE,d0		| ***read parity err status***
	cmpb	d0,d1			| parity error status as exp'd?
	beq	170$			|
	lea	par_st_err_txt,a4
	lea	170$,a6
	jra	error$			| error! parity error status not
170$:					| correct after reading bad parity!
	movb	#0,MERR_ADDR		| ***write mem err reg to clr***
	lea	180$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
180$:
	aslb	#1,d3			| next write pattern
	bne	110$
	movb	#0,MERR_BASE	| clr parity err register
	movb	#0,a5@		| write correct parity in last byte
	addql	#1,a5			| next byte of A<1,0>=00,01,10,11
	andb	#0x0f,d1
	asrw	#1,d1
	bne	104$			| last byte address to do?
	movb	#0,MERR_BASE		| clear parity int register
	movb	#0,MERR_ADDR		| clear memory err register
	movl	#0,0x1000		| set good parity in test address

| 	Size memory. Use Bus Error (nonexistant
|	memory timeout) trap to determine size.  

	lea	mem_sizing_txt,a4	| "Sizing memory..."
	lea	214$,a6
	jra	print$
214$:
	lea	220$,a0
	movl	a0,8			| setup bus err vector
	movl	#0x200000,a5		| start at top of 2 MBytes
216$:
	movb	a5@,d0			| ***attempt to read address***
	addl	#0x100000,a5		| next 1 MByte
	cmpl	#0x2000000,a5		| at 32 Bbytes yet?
	bne	216$			| if not & not bus err
220$:
	movl	a5,d5			| save top of memory +1
	moveq	#20,d0
	movl	d5,d3			| shift to make MBytes
	asrl	d0,d3
	movl	d3,MEM_size		| save memory size in page RAM
230$:
#endif SIRIUS
#ifdef SIRIUS 		
|*****************************************************************************
|       sirius ecc test
|
|       verify that by forcing a single bit error that the data gets corrected.
|       also verify in 2nd pass through the loop that a forced 2 bit error
|       does cause a syndrome code but does not correct the  bad bits.
|******************************************************************************

Test_0C:
        movb    #~0xC,d7                | 0xd > led display
        lea     Test_ecc_txt,a4
        lea     10$,a6
        jra     test$                   | display msg and leds
10$:
        lea     20$,a6                  | get return address
        jra     loop$                   | init top of loop pntr
20$:
        movl    #0x80000000,d3          | get data pattern
        movl    #0x01000000,d5          | get 1st diag reg pattern
        movl    #0xc000,d6              | get 2nd diag reg pattern
        movb    #0x8f,d4                | get expected syndrome code
        movw    #1,d2                   | get loop count
        movl    #ECC_MEM_BASE,a2        | mem ecc enb reg address
	movw	#0x40,a2@		| wayne says resets UE if is one
	movl	#0,a2@(4)		| reset syndrome reg
	movl	#MERR_BASE,a3
	bclr	#6,a3@
|
|       read syndrome register and report error is the syndrome code is not what
|       is expected.
|
ecc1:
        movl    #ECC_MEM_BASE,a2        | mem ecc enb reg address
        movl    #MERR_BASE,a3
	movl	#0,a5			| get memory address
        movl    #0,a5@                  | write data pattern to memory
        movl    #0x00,a5@(4)
        movl    d5,a2@(8)               | write cb to 1st half diag reg
        movl    d6,a2@(0xc)             | write cb to 2nd half diag reg
 
        movl    #ECCDIAG2,d0            | get diag mode bit
        orw     #0xc0,d0                | or in bd & ecc enb bits
        movw    d0,a2@                  | or diag mode into ecc enb reg
 
        movl    a5@,d0                  | read memory (creates the error)
        movl    a2@(4),d0               | read syndrome reg
        andl    #SYNMSK,d0              | get only cb bits
        roll    #8,d0                   | put cb code into lsb
	movl	a5@,d1			| get 1st data word
	nop
	movl	a5@(4),d5		| get 2nd data word

        movw    #0x40,a2@
        movl    #0,a2@(4)               | reset syndrome latch enable
	movl	#0,a3@(4)		| reset mem err reg

        cmpb    d0,d4                   | cb code = exptected ?
        beq     ecc2                    | yes, branch
        lea     ce_err_txt,a4           | get address of message
        lea     ecc2,a6                  | get return address
        jra     error$                  | msg "Err 11: Syndrome error exp 0x%d1
                                        |      rec 0xd0"
|
|       for single bit errors this memory word should have changed
|       for double bit error this location should not be chanded
|
ecc2:
	movl	a5@,d0			| get memory pattern
	btst	#0,d2			| if ue test use data from memory loc	
	beq	ecc2a			| else use data in d1
	movl	d1,d0
ecc2a:
        cmpl    d3,d0                   | = expected ?
        beq     ecc3                    | yes, branch
        lea     ecc_1_txt,a4            | get address of message
        lea     ecc3,a6
        jra     error$                  | msg "Err 11: single bit correction err
					| exp 0x%d3 rec 0x%d0
|
|       check 2nd long word in memory for no change
ecc3:
        movl    a5@(4),d0               | get memory pattern
	btst	#0,d2			| if ue test use data from mem loc
	beq	ecc3a			| else use data in d5
	movl	d5,d0	
ecc3a:
        cmpl    #0,d0                   | = expected ?
        beq     ecc4                    | yes, branch
        clrl    d3
        lea     ecc_1_txt,a4            | get address of message
        lea     ecc4,a6                 | get return address
        jra     error$                  | msg "Err 11: single bit correction err
                                        | exp 0x%d3 rec 0x%d0 
ecc4:
|***
	bset	#bit_tst_err,d7		| reset error flag
|***
        lea     10$,a6
        jra     loop$end                | go check end of loop options
10$:
|
|       init registers for 2 bit error check
|
        clrl    d3 	                | get expected data pattern 2nd pass
        movl    #0x01000200,d5          | init 1st diag reg pattern
        movl    #0x0c00,d6              | init 2nd diag reg pattern
        movl    #0xfc,d4                | init expected syndrome reg data
        dbra    d2,ecc1                 | loop
	movl	#0,a5@			| reinitialize data pattern
        movl	#0,a5@(4)
| take out for space reasons...
#ifdef	SIRIUS	
#ifdef	NOT_USEFUL
|----------------------------------------------------------------------------
|       <Cache Data Write/Read Test>
|
| Write and read each address of the cache data blocks in device control
| space. This test writes the address with a pattern then inverts the
| pattern and writes the next address, then reads back the original address
| and compares it with the noninverted pattern.
|
|       Upon error loop on failing cache data long word address.
|
|       Test type:      FATAL if error, loops forever
|       LED display:    0x8D upon error; 0x0D if no error

Test_0D:
        movb    #~0x0D,d7               | test #
        lea     Test_0D_txt,a4          | test descriptor text
	lea	40$,a6
        jra     test$
40$:
        movl    #Test_patt,d5           | initialize test pattern
50$:
        movl    d5,d1                   | working test pattern
        movl    #NCACHEWDS - 2,d4       | # of addresses to test
        movl    #CACHEDOFF,a5
54$:
	lea	60$,a6
        jra     loop$                   | <<<TOP OF TEST LOOP>>>
60$:
        movsl   d1,a5@                  | ***write cache data address***
        notl    d1                      | complement pattern
        addl    #CACHEDINCR,a5            | next cache data address
        movsl   d1,a5@                  | ***write next page address***
        subl    #CACHEDINCR,a5            | original cache data address
        movsl   a5@,d0                  | ***read page address***
        notl    d1                      | recomplement pattern
        movl    d1,d2
        eorl    d0,d2                   | xor of good and bad
        beq     70$                     | if write  = read data
        lea     xor_rd_err_txt,a4
	lea	70$,a6
        jra     error$                  | ERROR! DATA COMPARE ERROR IN
70$:                                    | WRITING AND READING CACHE DATA
                                        | ADDRESS!
	lea	80$,a6
        jra     loop$end                | <<<BOTTOM OF TEST LOOP>>>
80$:
        addl    #CACHEDINCR,a5          | next cache data address
        dbra    d4,54$
        rorl    #8,d5                   | next modulo 3 pattern set
        cmpl    #patt_end,d5            | last pattern?
        bne     50$                     | if not
#endif	NOT_USEFUL
|--------------------------------------------------------------------------
|       <Cache Data 3-Pattern Test>
|
|       Three passes of write/read tests are performed in the cache data
|       control space.
|
|       1st pass        2nd pass        3rd pass
|       --------        --------        --------
|       A5972C5A        5AA5972C        2C5AA597
|       5AA5972C        2C5AA597        A5972C5A
|       2C5AA597        972C5AA5        5AA5972C
|
|       Upon error, the entire pattern write/read throughout the cache
|       data space is looped.  This test is not ideal for scope looping!

Test_0D:
        bset    #bit_print_all,d7       | print all errors in this test
        movb    #~0x0D,d7               | test #
        lea     Test_0E_txt,a4          | test descriptor text
        lea     6$,a6
        jra     test$
6$:
        movl    #Test_patt,d2
11$:
        lea     12$,a6
        jra     loop$                   | <<<TOP OF TEST LOOP>>>
12$:
        movl    #NCACHEWDS-1,d4         | # of cache data addresses
        movl    #CACHEDOFF,a5           | cache data base address
14$:
        movl    d2,d3                   | init pattern generator
        moveq   #3,d5                   | modulo 3 pattern count
16$:
        movsl   d3,a5@                  | ***write cache data address***
        subql   #1,d4                   | decrement count
        ble     18$                     | if done
        addl    #CACHEDINCR,a5          | next cache data address
        rorl    #8,d3
        subql   #1,d5
        bne     16$
        bra     14$
18$:
        movl    #CACHEDOFF,a5           | cache data base address
        movl    #NCACHEWDS-1,d4            | number of byte reads to do
20$:
        movl    d2,d3                   | initial pattern generator
        moveq   #3,d5                   | modulo 3 pattern count
24$:
        movsl   a5@,d0                  | ***read cache data address***
        movl    d3,d1
        cmpl    d0,d1                   | write pattern = read?

        beq     34$
        lea     mem_rd_err_txt,a4
        lea     34$,a6
        jra     error$                  | error! cache data read not
34$:                                    | equal pattern written!
        addl    #CACHEDINCR,a5          | next  address
        rorl    #8,d3                   | next pattern
        subql   #1,d4                   | last RAM address?
        ble     40$
        subql   #1,d5                   | decrement modulo 3 pattern count
        bne     24$
        bra     20$
40$:
        lea     50$,a6
        jra     loop$end                | <<<BOTTOM OF TEST LOOP>>>
50$:
        rorl    #8,d2                   | next modulo 3 pattern set
        cmpl    #patt_end,d2            | last pattern?
        bne     11$                     | if not
        bclr    #bit_print_all,d7


#ifdef	NOT_USEFUL
|----------------------------------------------------------------------------
|       <Cache Tags Write/Read Test>
|
| Write and read each address of the cache tags in device control
| space. This test writes the address with a pattern then inverts the
| pattern and writes the next address, then reads back the original address
| and compares it with the noninverted pattern.
|
|       Upon error loop on failing cache tags long word address.
|
|       Test type:      FATAL if error, loops forever
|       LED display:    0xa0 upon error; 0x20 if no error

Test_0F:
        movb    #~0x0F,d7               | test #
        lea     Test_0F_txt,a4          | test descriptor text
	lea	10$,a6
        jra     test$
10$:
        movl    #Test_patt,d5           | initialize test pattern
50$:
        movl    d5,d1                   | working test pattern
        movl    #NCACHETGS  - 2,d4      | # of addresses to test
        movl    #CACHETOFF,a5
54$:
	lea	60$,a6
        jra     loop$                   | <<<TOP OF TEST LOOP>>>
60$:
        andl    #CACHETMASK,d1          | strip off nonrelevant bits
        movsl   d1,a5@                  | ***write cache tags address***
        notl    d1                      | complement pattern
        andl    #CACHETMASK,d1          | strip off nonrelevant bits
        addl    #CACHETINCR,a5          | next cache tags address
        movsl   d1,a5@                  | ***write next page address***
        subl    #CACHETINCR,a5          | original cache tags address
        movsl   a5@,d0                  | ***read tags address***
        notl    d1                      | recomplement pattern
        andl    #CACHETMASK,d1          | strip off nonrelevant bits
        andl    #CACHETMASK,d0          | strip off nonrelevant bits
        movl    d1,d2
        eorl    d0,d2                   | xor of good and bad
        beq     70$                     | if write  = read tags data
        lea     xor_rd_err_txt,a4
	lea	70$,a6
        jra     error$                  | ERROR! DATA COMPARE ERROR IN
70$:                                    | WRITING AND READING CACHE DATA
                                        | ADDRESS!
	lea	80$,a6
        jra     loop$end                | <<<BOTTOM OF TEST LOOP>>>
80$:
        addl    #CACHETINCR,a5          | next cache tags address
        dbra    d4,54$
        rorl    #8,d5                   | next modulo 3 pattern set
        cmpl    #patt_end,d5            | last pattern?
        bne     50$                     | if not
#endif	NOT_USEFUL
|-----------------------------------------------------------------------------
|       <Cache Tags 3-Pattern Test>
|
|       Three passes of write/read tests are performed in the cache tags
|       control space.
|
|       1st pass        2nd pass        3rd pass
|       --------        --------        --------
|       A5972C5A        5AA5972C        2C5AA597
|       5AA5972C        2C5AA597        A5972C5A
|       2C5AA597        972C5AA5        5AA5972C
|
|       Upon error, the entire pattern write/read throughout the cache
|       tags space is looped.  This test is not ideal for scope looping!

Test_10:
        movb    #~0x0E,d7               | test #
        lea     Test_10_txt,a4          | test descriptor text
        lea     6$,a6
        jra     test$
6$:
        movl    #Test_patt,d2
11$:
        lea     12$,a6
        jra     loop$                   | <<<TOP OF TEST LOOP>>>
12$:
        movl    #NCACHETGS-1,d4         | # of cache tags addresses
        movl    #CACHETOFF,a5           | cache tags base address
14$:
        movl    d2,d3                   | init pattern generator
        moveq   #3,d5                   | modulo 3 pattern count
16$:
        movl    d3,d1
        andl    #CACHETMASK,d1          | strip off nonrelevant bits
        movsl   d1,a5@                  | ***write cache tags address***
        subql   #1,d4                   | decrement count
        ble     18$                     | if done
        addl    #CACHETINCR,a5          | next cache tags address
        rorl    #8,d3
        subql   #1,d5
        bne     16$
        bra     14$
18$:
        movl    #CACHETOFF,a5           | cache tags base address
        movl    #NCACHETGS-1,d4            | number of byte reads to do
20$:
        movl    d2,d3                   | initial pattern generator
        moveq   #3,d5                   | modulo 3 pattern count
24$:
        movsl   a5@,d0                  | ***read cache tags address***
        movl    d3,d1
        andl    #CACHETMASK,d1          | strip off nonrelevant bits
        andl    #CACHETMASK,d0          | strip off nonrelevant bits
        cmpl    d0,d1                   | write pattern = read?
        beq     34$
        lea     mem_rd_err_txt,a4
        lea     34$,a6
        jra     error$                  | error! cache tags  read not
34$:                                    | equal pattern written!
        addl    #CACHETINCR,a5          | next  address
        rorl    #8,d3                   | next pattern
        subql   #1,d4                   | last RAM address?
        ble     40$
        subql   #1,d5                   | decrement modulo 3 pattern count
        bne     24$
        bra     20$
40$:
        lea     50$,a6
        jra     loop$end                | <<<BOTTOM OF TEST LOOP>>>
50$:
        rorl    #8,d2                   | next modulo 3 pattern set
        cmpl    #patt_end,d2            | last pattern?
        bne     11$                     | if not
#endif	SQUEEZE
|************************************************************************
|       memory size 
|
|	use Bus Error timeout to determine if memory board
|	is present or not.
|************************************************************************
mem_size:
        movl    #0x800,sp               | init stack ptr
        lea     buserr,a0               | set up vector return addr
        movl    a0,8                    | set trap vector
        lea     mem_sizing_txt,a4       | msg "Sizing memory .."
        lea     10$,a6
        jra     print$                  | go display msg
10$:
        lea     ECC_MEM_BASE,a5         | get address of ecc enable reg
        clrl    d6                      | init buss error flag
        clrl    d3
        movw    #3,d4                   | init loop count
        movw    #0x40,d2                | enb mem bd and init base reg
        clrl    d1                      | init mem brd # reg
memsz1:
	clrl	d0			| reset d0
        movw    a5@,d0                  | read ecc mem enable reg
memsz2:
        cmpb    #0,d6                   | buss error ocured ?
        beq     memsz3                  | no, mem board present go check for 
					| next one
        bra     memsz7                  | go check next board
memsz3:
|        addqb   #1,d1                   | inc board # in # reg
memsz4:
        btst    #9,d0                   | 8 meg board
        bne     memsz5                  | yes, branch
        lea     mem_invalid_txt,a4      | msg addr
	lea	10$,a6			| get return address
        jra     error$                  | msg "invalid mem size"
10$:
        bra     memsz7
memsz5:
        movw    d2,a5@                  | init mem enb reg
        addql   #0x2,d2                 | step base address
        addl    #MEG8,d3                | get max mem address
memsz7:
	clrl	d6			| reset error flag
	addw	#0x40,a5		| step to next highest board #
        dbra    d4,memsz1               | go check presence of next mem bd
 
        lea     unex_bus_err,a0         | restore unexpected bus error
        movl    a0,8
	movw	#20,d0
	asrl	d0,d3			| shift down for msg
50$:
        movl   d3,MEM_size             | save memory size in map ram
	movl	d3,d5			| store memory size
 	bra	check_sw		| go read the eeprom	
|***************************************************************************
|	bus error routine for sirius memory size check
|***************************************************************************
buserr:
        addql   #1,d6                   | inc buss error flg
        bra     memsz2                  | return to size routine
#endif	SIRIUS


|	Check if diag switch = 1. 
check_sw:
230$:
	moveq	#20,d0			| get shift count
	lea     unex_bus_err,a0         | restore unexpected bus error service
        movl    a0, 8

	movsb	ENABLEOFF,d2		| read diag switch
	btst	#DIAGSW,d2		| diag sw = 0?
#ifdef M25
	bne	250$			| if not, test all of memory

        movb    EEPROM_MEM_SZ,d2        | ***read EEPROM for mem sz
        beq     end_test                | if 0, don't test memory
        cmpb    d2,d3                   | sz to test <= memory size?
        bge     250$                    | if not
        movb    d2,d3
#else
	beq	end_test		| if yes, don't test memory
#endif M25
250$:
	movl	d3,d5
	lsll	d0,d5			| expand to memory address

#ifndef SIRIUS 
|-----------------------------------------------------------------------------
| Memory Test
|
| The MMU is setup to unity map virtual memory space to physical.
| Memory is then written with a repeated three long work pattern
| sequence.  Memory is then read back and compared with parity checking
| and parity error interrupts enabled.  The three long word pattern is
| then repeated with the pattern order shifted. 
|
|
|	1st pass	2nd pass	3rd pass
|	--------	--------	--------
|	A5972C5A	5AA5972C	2C5AA597
|	5AA5972C	2C5AA597	A5972C5A
|	2C5AA597	972C5AA5	5AA5972C
| These patterns ensure odd and even parity are written in every parity bit in
| memory as well as memory data bit and  that memory addressing is tested
| as well.
Test_10:
	bset	#bit_print_all,d7
        cmpl    #0x2000000,d5           | must not write over our mapped pages 
        blt     1$ 
        subl    #0x20000,d5             | adjust for size 
1$: 
	movb	#~0x10,d7		| 0xF > LED display
	lea	Test_11_txt,a4
	lea	50$,a6
	jra	test$			| output test number and name
| Thirdly, set up parity error trap service and enable it while testing
50$:
	lea	83$,a0		
	movl	a0,nmi_vector		| point parity err to this routine

	movb	#PARCHECK+EN_PARINT,MERR_BASE
					| ***enable parity err interrupts***
	movl	#Test_patt, a0		| 1st write generator pattern

|	Write modulo three pattern in memory

60$:
	movl    a0,save_a0              | save a0 please!
	lea	64$,a6
	jra	loop$			| <<<TOP OF TEST LOOP>>>
64$:
	lea	low_mem_addr,a5		| starting memory address to test
70$:
	movl	a0,d3			| init pattern generator
	movl	d3,a5@+			| ***write long word pattern***
	cmpl	d5,a5			| at end of memory?
	beq	78$
	rorl	#8,d3			| next long word pattern
	movl	d3,a5@+			| ***write long word pattern***
	cmpl	d5,a5			| end of write to memory
	beq	78$
	rorl	#8,d3			| next long word pattern
	movl	d3,a5@+			| ***write long word pattern***
	cmpl	d5,a5			| at end of memory?
	bne	70$

|	Read back and compare with pattern generator.

78$:
	lea	low_mem_addr,a5		| starting memory address to test
80$:
	movl	save_a0,d1		| init pattern generator
	moveq	#2,d3			| modulo 3 count
82$:
	movl	a5@,d0			| ***read long word address***
	cmpl	d1,d0			| write = read pattern?
	beq	86$
	lea     xor_rd_err_txt,a4
	bra	85$			| error!!  A write/read data compare
83$:
        movl    #0x800,sp               | reinit stack pointer
	clrl	d2
	movb	MERR_BASE,d2		| ***rd parity err status***
	movl	#0,MERR_ADDR		| ***clr parity err status***
	lea     mem_par_err_txt,a4
	btst	#7,d2			| parity interrupt bit set?
	bne	85$			| if not?!?
	lea	nmi_ques_int_txt,a4
85$:
	bclr    #bit_print_all,d7
	movl    d1,d2                   | copy to d2 for xor
        eorl    d0,d2
	lea	82$,a6
	jra	error$			| Error! An nmi interrupt without
					| parity err interrupt status!!
86$:
	btst    #bit_tst_err,d7         | 1st error in test?
        bne     87$                     | if not first error

	movsb   ENABLEOFF,d0            | read diagnostic switch
        btst    #DIAGSW,d0              | is switch = ON?
	bne     82$                     | If yes, try again
	lea     continue_txt,a4
	lea     87$,a6
        jra     print$
87$:
	bset    #bit_print_all,d7
	bset    #bit_tst_err,d7
	addl	#4, a5			| Increment memory address
	rorl	#8,d1			| next pattern
	cmpl	d5,a5			| at end of memory?
	beq	88$
	dbra	d3,82$			| modulo three yet?
	bra	80$
88$:
	lea	89$,a6
	jra	loop$end		| <<<BOTTOM OF TEST LOOP>>>
89$:
	movl	save_a0,d4
	rorl    #8,d4
	movl	d4,a0			| next pattern generator
	cmpl	#patt_end,a0		| last pattern +1?
	bne	60$			| next memory pass
#endif  SIRIUS 
#ifdef	SIRIUS 
|************************************************************************
|       Memory Modulo 3's Test
|
|	using a modulo 3 test pattern write the data into memory from the
|	table this also causes a modulo 3 pattern in ecc memory with
|	the following pattern 0, ff, ff, ff, ff, 0, ff, 0, ff.
|************************************************************************
        .data
mod3tbl:
        .long   0xb1554efa                | 1st pat ecc = 0
        .long   0x473a5a59                | 2nd pat ecc = 0
        .long   0xb1555b2a                | 1st pat ecc = ff
        .long   0x3ea7a2e5                | 2nd pat ecc = ff
        .long   0x4eaaa4d5                | 1st pat ecc = ff
        .long   0xc1585d1a                | 2nd pat ecc = ff
        .long   0xb1554efa                | 1st pat ecc = 0
        .long   0x473a5a59                | 2nd pat ecc = 0
        .long   0x4eaaa4d5                | 1st pat ecc = ff
        .long   0xc1585d1a                | 2nd pat ecc = ff
 
        .text
Test_11:
        movsb   ENABLEOFF,d2            | read diagnostic switch
        btst    #DIAGSW,d2              | is switch = ON?
        beq     6$                      | if not
        bset    #bit_print_all,d7       | yes, set no loop on error
6$:                                     | and print all errors
        movb    #~0x0F,d7               | get led display
	movl   MEM_size,d3
| FIXME! if 32 megs then test only 31 megs
| look at mem init below and you'll get the idea
	cmpl	#MEG32,d3		| is it?
	bne	8$			| cant be > 32meg so go on
	movl	#MEG31,d3		| at 31 megs we are safe
	movl	#0x1f00000,d5
8$:	
        lea     Test_11_txt,a4          | get msg address
        lea     15$,a6                  | return address
        jra     test$                   | display test number
15$:
        lea     mod3tbl,a0              | get table address
        movl    #0x900,a1               | memory address
        movw    #9,d0
	movl	#FC_SP,d1		| get prom function code
	movc	d1,sfc
20$:
	movsl	a0@+,d1
	movl	d1,a1@+			| store table into memory
        dbra    d0,20$
	moveq	#FC_MMU,d0		| reinit funciton code to mmu
	movc	d0,sfc
	movc	d0,dfc
	movl	#15,ce_error		|  set ce count
	clrl	ue_error		| clear UE count
        lea     30$,a6                  | get return address
        jra     loop$                   | set up top of loop address
30$:
	clrw    save_d4                 | index pointer
|
|       write data patterns to memory getting data from table
|
mod3tsta:
	movl	#low_mem_addr,a5	| get starting address
        movl    #0x900,a0               | get table address
mod3tst1:
        movw    #2,d3                   | table length count
        movw    save_d4,d2

mod3tst1a:
        movl    a0@(0,d2:w),a5@+        | write 1st pattern
        movl    a0@(4,d2:w),a5@+        | write 2nd long word in pattern
        addl    #8,d2                   | step pointer
        cmpl    d5,a5                   | end of memory ?
        bge     mod3tst2                | yes, goto read
        dbra    d3,mod3tst1a            | continue writing
        bra     mod3tst1                | start with next pattern
|
|	init for read data compare loop
|
mod3tst2:
	movb	#0x10,MERR_BASE		| init mem err reg
|
|	turn on ecc on all boards in the system.
|
	movl	d5,d1
	movw	#23,d0
	lsrl	d0,d1			| get number of boards
	subqw	#1,d1
	movl	#ECC_MEM_BASE,a2	| get enb reg address
10$:
	movw	a2@,d0			| get enb reg contents
	orw	#0xc0,d0		| enable ecc
	movw	d0,a2@			| write to enable reg
	addl	#0x40,a2 		| set to next mem board
	dbra	d1,10$			| go enable ecc on next mem bd.
	movl	#ECC_MEM_BASE,a2	| reinit ecc mem enable reg addr

|
|       read memory compare data from table to data from memory
|
	movl	#low_mem_addr,a5	| init starting address
	movl	#0x900,a0		| get table address
mod3tst3:
        movw    save_d4,d2              | table length
	movw	#6,save_d6		| save count
mod3tst4:
        movl    a5@+,d0                 | read data
        movl    a0@(0,d2:w),d1          | get expected data
        cmpl    d0,d1                   | expected data = received data
        beq     mod3tst5                | yes, branch
        lea     mem_rd_err_txt,a4       | address of message
        subql    #4,a5                  | adjust to correct address
        lea     10$,a6                  | get return address
        jra     error$                  | display message
10$:
        addql    #4,a5                  | get next address
| 	re-init a0 here cause gets destroyed in print$ MJC 
	movl    #0x900,a0               | get table address
mod3tst5:
	clrl	d1
	movb	MERR_BASE,d1		| get mem err reg contents
	andb	#3,d1			| check for "UE or CE"
	beq	mod3tst6a		| no, branch
	jra	eccmod3			| go determine UE or CE and report err
mod3tst6:
	movl	#0x900,a0		| restore table address
mod3tst6a: 
        cmpl    d5,a5                   | end of memory ?
        bge     mod3tst7                | yes, go check syndrome reg
        addql   #4,d2                   | step index
	subqw	#1,save_d6
	bne	mod3tst4
        bra     mod3tst3
mod3tst7:
        addw    #8,save_d4              | step to next pattern
        cmpw    #0x18,save_d4           | last pattern
        blt     mod3tsta                | no, go test next pattern
        lea     10$,a6                  | get return address
        jra     loop$end                | set bottom of loop
10$:
mod3tst8:
	bra	end_test 
|*************************************************************************
|       check syndrome reg for error (subroutine)
|*************************************************************************
eccmod3:
	movl	#ECC_MEM_BASE,a2	| get enable reg address
	movl	a5,d0			| get address
	movb	#23,d4
	lsrl	d4,d0			| get syndrome reg addr
	lsll	#6,d0
	addw	d0,a2			| get syndrome reg address
	movl	a2@(4),d0		| get syndrome reg contents
|
|	clear errors
|
	movw	a2@,d4			| read enb reg
	movw	#0x40,a2@		| clear ue
	movw	d4,a2@			| put enable reg contents back
	movl	#0,a2@(4)		| clear syndrome reg
	movl	#0,MERR_ADDR		| reset memory err reg on cpu
	
	movl	#ue_error,a4		| get UE count
	cmpl	#2,a4@			| > 1
	bge	10$			| yes, go to scope loop
	btst	#1,d1			| UE error ?
	beq	eccCE_err		| no, must be CE error then branch
	addqb	#1,a4@			| inc error count
|
|	UE error, if 1st error print message; else loop on error
|
        lea     syn_err_txt,a4          | address of message
        lea     10$,a6		        | get return address
        jra     error$                  | display message
10$:

	movl	#0x900,a0		| reset table address
	subql	#4,a5			| dec address to failing address
	jra	mod3tst4		| go check failing address again.
eccCE_err:
	movl	#ce_error,a4		| get counter address
	tstl	a4@			| we at 0?
	beq	90$
	subql	#1,a4@			| dec ce count
        lea     syn_err_txt,a4          | address of message
        lea     90$,a6             	| get return address
        jra     error$                  | display message
90$:
	jra	mod3tst6		| go coninue testing	
#endif	SIRIUS 

end_test:
#ifdef	SIRIUS
|and now disable ce reporting
        movb    #0x40,MERR_BASE
        movl    #0,MERR_ADDR            | reset memory err reg on cpu
#endif	SIRIUS
	bclr	#bit_print_all,d7	| clr print all/no loop switch
	lea	unex_par_int,a0
	movl	a0,nmi_vector		| restore nmi service
	
	btst	#bit_fail,d7		| did selftest fail?
	beq	10$
	lea	test_fail_txt,a4
	lea	20$,a6
	jra	print$			| "Selftest failed"
10$:
	lea	test_pass_txt,a4
	lea	20$,a6
	jra	print$			| "Selftest passed successfully"
20$:
	btst	#bit_burn_in,d7		| doing burn in test?
	bne	30$			| if yes  TEMP HACK MIKE
	bra	40$			| if not
30$:
#ifdef  M25
	jra     Test_00
#else
	jra	Start			| if burn in test, restart
#endif	M25
40$:
	movl	MEM_size,d2
esckey:
	movl	#20,d0
	asll	d0,d2			| setup saved memory size

| Initialize stack pointer and fill all of memory with 0xFFFF,
| the unimplemented instruction code
| IMPORTANT! d2 must be left alone until we get to mapmem below!!!
	movl	d2,d1
#ifdef SIRIUS
	cmpl    #0x2000000,d1           | must not write over our mapped pages
        blt     47$
        subl    #0x20000,d1             | adjust for size
47$:
        subl    a5,a5                   | start a address 0
        movl    #0xFFFFFFFF,d0          |
50$:
        movl    d0,a5@+                 | store it in memory
        cmpl    d1,a5                   | at top of memory yet?
        bne     50$                     | if not

	cmpl    #0x1fe0000,d1		| Is there 32 Mb of memory
	blt     54$			| If not, initialization is done

	moveq	#0xf, d1		| Else, we missed upper 16 pages
	movl	#0xc0000ff0, d3		| Get page entry for 32Mb - 128K
	movl	#MEMINIT_PAGE, d4	| Use the same page for 16 pages of mem
	movl    #PAGEOFF,a0		| Get the Page Map offset
52$:
	movsl   d3,a0@(0,d4:L)          | Set Page to appropriate memory
	movl    d4, a5                  | Get starting address

	movl	#0x7ff, d0		| Number of long words in page
53$:
	movl	#0xFFFFFFFF, a5@+	| Initialize it to 0xFFFFFFFF
	dbra	d0, 53$

	addl    #1, d3			| Increment Page entry to next 8K mem
	dbra	d1, 52$			| Do this for the remaining 16 pages
54$:
#else
	cmpl	#0x2000000,d1		| must not write over our mapped pages
	blt	47$
	subl	#0x100000,d1		| adjust for size
47$:

	subl	a5,a5			| start a address 0
	movl	#0xFFFFFFFF,d0		|
50$:
	movl	d0,a5@+			| store it in memory
	cmpl	d1,a5			| at top of memory yet?
	bne	50$			| if not
#endif SIRIUS
| for sirius, enable ecc for all mem brds but disable ce reporting
#ifdef	SIRIUS	
        movl    d2,d1			| d2 is setup from above
        movl    #23,d0
        lsrl    d0,d1                   | get number of boards
        subqw   #1,d1
        movl    #ECC_MEM_BASE,a2        | get enb reg address
enbl_ecc:
        movw    a2@,d0                  | get enb reg contents
        orw     #0xc0,d0                | enable ecc & brd
        movw    d0,a2@                  | write to enable reg
        addl    #0x40,a2                | set to next mem board
        dbra    d1,enbl_ecc             | go enable ecc on next mem bd.
|and now disable ce reporting
	movb	#0x40,MERR_BASE	
#endif	SIRIUS
	movb    #0,INT_BASE             | clear interrupt enable bits
	movw	#EVEC_RESET,d3		| Fake Reset fvo
	movsb	ENABLEOFF,d1		| read diag switch state
	btst	#DIAGSW,d1		| is diag switch set?
	beq	100$			| if not, proceed to Unix boot
	movw	#EVEC_BOOT_EXEC,d3      | Fake Boot Diag Exec fvo
	lea     _menu_msg,a4            | "Optional Tests
        lea     55$,a6                  | Menu"
        jra     print$
55$:
	lea	m_op_tests,a4		| "(enter e for echo mode)"
	lea	60$,a6			 
	jra	print$			
60$:
	movl	#delay_10_sec,d0	| delay count for ~10 seconds
	bclr	#31, d3			| Clear Port B flag bit
70$:
#ifdef	M25
	movb	UARTACNTL,d1		| read Port A RX status
#else
	movsb   UARTACNTL,d1            | read Port A RX status
#endif M25
	btst	#RXREADY,d1		| an input character entered?
	bne	80$			| if yes

	moveq   #0xf, d1
71$:    dbra    d1, 71$                 | Wait for SCC to recover

#ifdef  M25
        movb    UARTBCNTL,d1    	| read Port B RX status
#else
        movsb   UARTBCNTL,d1            | read Port B RX status
#endif M25
        btst    #RXREADY,d1             | an input character entered?
        bne     82$                     | if yes

	subql	#1,d0			| decrement delay count
	bgt	70$			| if no timeout yet
	bra	100$			| timed out
80$:
	moveq   #0xf, d0
81$:    dbra    d0, 81$                 | Wait for SCC to recover
 
#ifdef M25
        movb    UARTADATA, d0           | Read the character
#else
        movsb   UARTADATA, d0           | Read the character
#endif M25
	jra	84$
82$:
	moveq   #0xf, d0
83$:    dbra    d0, 83$                 | Wait for SCC to recover

	bset    #31, d3                 | Set bit to flag this is Port B
#ifdef M25
        movb    UARTBDATA, d0           | Read the character
#else    
        movsb   UARTBDATA, d0           | Read the character
#endif M25
84$:
	andb    #0x7f, d0               | Strip off parity bit
        cmpb    #0x65, d0               | Is it an 'e'?
        bne     85$                     | If not, do not set echo bit
 
        bset    #bit_echo_mode, d7      | Else, set echo bit
        lea     echo_msg, a4            | "Echo mode: Output will echo
        lea     85$, a6                 | to Video"
        jra     print$
85$:
        lea     90$,a6
        jra     UARTinit                | reinit UART prior to return
90$:
	movw	#EVEC_MENU_TSTS,d3      | Fake vfo for menu tests
100$:					| from serial port A	
	movl	d2,a6			| memory size
	moveml	#0xFFFF,sp@-		| save all registers
	moveq	#~L_SETUP_MAP,d0	| 
	movsb	d0,LEDREG		| tell LEDs we're setting up MAP
	pea	a6@			| push top of mem arg on stack
	jbsr	_mapmem			| for Map Memory call
	addql	#4,sp			
	movl	a6,g_memoryworking	| total amt of working memory
	movl	d0,g_memoryavail	| available memory after we steal
	moveml	sp@+,#0xFFFF		| restore all registers
	movl	a6,g_memorysize		| save memory size
	movl	#0,g_memorybitmap	| no memory bit map yet: FIXME
| added to make sure we know when we exit mapmem.  MJC
	movb	#L_SETUP_MAP,d0		| invert the last led pattern
	movsb   d0,LEDREG		| and turn it off
	movb	#0x01, g_sccflag	| Initialize SCC flag to Port A
	btst    #bit_echo_mode, d7      | should we set the echo mode flag
        beq     110$                    | If not, do not set flag
        movb    #0x12, g_diagecho       | Else, set the flag
110$:
	btst	#31, d3			| Check the SCC Port flag
	beq	120$			| If not set, this is Port A
	movb    #0x02, g_sccflag        | Else, set the flag for Port B
120$:
	movw	d3,INITSP-6		|
	movl	a5,a6			| restore memory size
	jra	_reset_common		| return to trap.s module

	.globl  _mod3write
|
|	_mod3write: Write the modulo 3 pattern test from address a0 to a1.
|                   Start with pattern in d1, and return ending patten in d0.
|
_mod3write:
        movl    sp@(4), a0       	| Get starting address
	movl    sp@(8), a1              | Get ending address
        movl    sp@(12), d1       	| Get starting pattern
1$:
	movl	d1, d0			| Init pattern generator
	movl	d0, a0@+		| Write pattern to memory
	cmpl    a0, a1                  | Check if at end of memory 
        beq     2$                      | If yes, exit
	rorl	#8, d0			| Next long word pattern
	movl    d0, a0@+                | Write pattern to memory
	cmpl    a0, a1                  | Check if at end of memory
        beq     2$                      | If yes, exit
        rorl    #8, d0                  | Next long word pattern
	movl    d0, a0@+                | Write pattern to memory
	cmpl	a0, a1			| Check if at end of memory
	bne	1$			| If not, keep writing
2$:
        rts

	.globl  _mod3read
| 
|       _mod3read: Read the modulo 3 pattern test from address a0 to a1. 
|                  Start with pattern in d1, and return ending patten in d0. 
|
_mod3read:
        movl    sp@(4), a0              | Get starting address
        movl    sp@(8), a1              | Get ending address
        movl    sp@(12), d4             | Get starting pattern
1$:
        movl    d4, d1                  | Init pattern generator
	lea     2$, a5			| Return point if no error
	bra     readrtn                 | Test a long word
2$:
	lea     3$, a5			| Return point if no error
        bra     readrtn                 | Test a long word
3$:
	lea     1$, a5			| Return point if no error
	bra	readrtn			| Test a long word

end_mod3:
        rts

|
|	readrtn: Check for errors and memory limits
|
readrtn:
	movl    a0@+, d0                | Read pattern from memory
        cmpl    d0, d1                  | Compare the pattern
        bne     1$                      | If not equal, set error flag
	rorl    #8, d1                  | Next long word pattern
        cmpl    a0, a1                  | Check if at end of memory
        jne     a5@                     | If not, continue test
	bra	2$			| Else, exit mod3read
1$:
	tstl	a0@-			| Back up address pointer
	movl	a0, g_mod3addr		| Save the memory address
	movl    d1, g_mod3exp           | Save the expected value
	movl    d0, g_mod3obs           | Save the observed value
	clrl    d0                      | Error: d0 = 0
2$:
	jra	end_mod3                | Return to end of mod3read

|----------------------------------------------------------------------------
| Unexpected bus error trap service
|
| Prints message giving pc at bus error then returns to top of loop
| address which is saved in the msp (master stack pointer).

unex_bus_err:
        bclr    #bit_tst_err,d7         | set exception err bit
	bclr	#bit_exc_err,d7		| set exception err bit 
        movsb   d7,LEDREG               | and display it with test #
        lea     unex_be_txt,a4
        lea     10$,a6
	movl	sp@(pc_index),d0	| get pc off stack	
        jra     print$                  | "unexpected bus error"
10$:
	movl	sp@(access_index),d0
	lea	access_txt,a4
	lea	20$,a6
	jra	print$			| print access address from stack
20$:
	clrl	d0
        movsb	BERRREG,d0	        | get pc at bus error
	lea	berr_txt,a4
	lea	30$,a6
	jra	print$			| print bus error register contents
30$:
        movl    #0x800,sp               | reinit stack pointer
	movw	#0x3700,sr		| to access master sp
	movl	sp,a0
	movw	#0x2700,sr
err:
	jra	a0@			| loop to top of current test

|----------------------------------------------------------------------------
| Unexpected parity error trap service
|
| Prints message giving pc at bus error then return to top of loop
| address which is saved in the msp (master stack pointer).

unex_par_int:
	bclr	#bit_tst_err,d7		| set exception err bit
	movsb	d7,LEDREG		| and display it with test #
#ifdef SIRIUS
	lea	unex_ecc_txt,a4		| get addr of ecc error msg
#else
	lea     unex_pe_txt,a4
#endif SIRIUS
	lea	10$,a6
	movl	sp@(pc_index),d0	| get pc at bus error
	clrl	d5
	movb	MERR_BASE,d5		| read parity error register
	jra	print$			| "unexpected bus error"
10$:
	movb	#0,MERR_ADDR		| ***clr parity interrupt***
	rts

|----------------------------------------------------------------------------
| Unexpected trap/vector service
|
| Prints message giving pc at bus error then returns to top of loop
| address which is saved in the msp (master stack pointer).

unex_trap_err:
	bclr	#bit_tst_err,d7		| set exception err bit
	movsb	d7,LEDREG		| and display it with test #
	lea	unex_trap_txt,a4
	lea	10$,a6
	movl	sp@(pc_index),d0	| get pc at bus error
	jra	print$			| "unexpected bus error"
10$:
	lea	trap_txt,a4
	clrl	d0
	movw	sp@(vector_index),d0	| get vector offset from stack
	andw	#0xfff,d0		| mask off all but vector offset
	lea	20$,a6
	jra	print$
20$:	
        movl    #0x800,sp               | reinit stack pointer
	movw	#0x3700,sr		| to access master sp
	movl	sp,a0
	movw	#0x2700,sr
	jra	a0@			| loop to top of current test


|-----------------------------------------------------------------------
| Bus Error Service for bootprom in prom

_diag_berr:
	jmp	a4@			| bus error handler
	
Berror:
        addql   #1,d3                   | Count one more error
        movl    #LEDQUICK,d0            | Pause to human speeds
LElong: subql   #1,d0
        bne     LElong
        jra     a0@                     | Rerun failing test


|-----------------------------------------------------------------------------
| Test Routine
|
|	Displays test number in diagnostic LEDs at rear edge of CPU board at
|	start of each test.  If diagnostic switch is on (1), outputs test name
|	text to Serial Port A;  otherwise, does not.  Returns a6@.
|
|	Calling sequence:	lea	test_text_address,a4
|				lea	return_address,a6
|				jra	test$
|

test$:
	moveq	#FC_MMU,d0		| setup sfc,dfc to MMU for "movs" 
	movc	d0,sfc			| instruction
	movc	d0,dfc
	movsb	d7,LEDREG		| output test # to LEDs
	movl	#0xFFFF,d0
10$:
	subql	#1,d0
	bgt	10$
	jra	print$			| print test text & return

|-----------------------------------------------------------------------------
| Top of Test Loop Routine
|
|	Saves PC return for looping on error in a4.
|
|	Calling sequence:	lea	return_address,a6
|				jra	loop$
|
loop$:
	movw	#0x3700,sr		| set master mode to use msp
	movl	a6,sp			| save top of loop address
	movw	#0x2700,sr		| in master sp
	jra	a6@			| return

|----------------------------------------------------------------------------
| Error Routine
|
|	Outputs test number (bits 4-0) with error bit (bit 7) set to
|	diagnostic LEDs at rear edge of CPU board.  Always returns
|	a6@.
|
|	Calling sequence:	lea	error_text_address,a4
|				lea	return_address,a6
|				jra	error$

error$:
	btst	#bit_print_all,d7	| print all errors?
	bne	10$			| if yes	
	btst	#bit_tst_err,d7		| 1st error in test?
	beq	20$			| if not first error
10$:
	movl	d0,a3
	moveq	#FC_MMU,d0		| setup sfc,dfc to MMU for "movs" 
	movc	d0,sfc			| instruction
	movc	d0,dfc
	bclr	#bit_tst_err, d7	| set test error bit(inverse)
	bset	#bit_fail,d7		| set accumulated error bit
	movsb	d7,LEDREG		| output test # to LEDs
	movl	a3,d0
	jra	print$			| print error message
20$:
	jra	a6@			| return

|----------------------------------------------------------------------------
| Bottom of Test Loop Routine
|
|	If fatal class error, loops forever.
|	If nonfatal class error and diag switch = 1, loops forever.
|

loop$end:
        btst    #bit_no_read,d7         | read char from port A?
        bne     40$
	moveq	#FC_MMU,d0
	movc	d0,sfc			| set sfc to MMU
#ifdef	M25
	movb	UARTACNTL,d0		| read SCC status
#else
	movsb   UARTACNTL,d0            | read SCC status
#endif M25
	btst	#RXREADY,d0		| char rec'd?
	beq     1$                      | if so

#ifdef	M25
	movb	UARTADATA,d0		| read char from Port A
#else
	movsb   UARTADATA,d0            | read char from Port A
#endif M25
	jra	2$			| Skip over reading Port B
1$:
#ifdef  M25
        movb    UARTBCNTL,d0            | read SCC status
#else
        movsb   UARTBCNTL,d0            | read SCC status
#endif M25
        btst    #RXREADY,d0             | char rec'd?
        beq     40$                     | if so
#ifdef  M25  
        movb    UARTBDATA,d0            | read char
#else
        movsb   UARTBDATA,d0            | read char
#endif M25
2$:
	cmpb	#op_skip_end,d0		|drop out of self test?
	bne	9$			| nope
#ifndef	MJC 
	bclr    #bit_burn_in,d7
	clrl	d0
	movsb	d0, CXREG		| Set Context Reg to 0

	movl	#SEGOFF, a5		| Set up Segment Map
	movl	#0xff, d4
3$:
	movsb	d0, a5@			| Write to Segment Map
	addl	#1, d0
	addl	#SEGINCR, a5
	dbra	d4, 3$

	movl	#MEMPAGE, d0
	movl    #PAGEOFF, a5            | Set up Page Map 
        movl    #0xfff, d4 
4$: 
        movsl   d0, a5@                 | Write to Page Map 
        addl    #1, d0 
        addl    #PAGEINCR, a5 
        dbra    d4, 4$

	movl    #PAGEOFF,a5
        lea     SCC_PAGE,a0
        movl    #SCC_BASE,d0
        movsl   a0,a5@(0,d0:L)          | page for SCC
        lea     INT_PAGE,a0
        movl    #INT_BASE,d0
        movsl   a0,a5@(0,d0:L)          | page for Interrupt Enable Register

#ifdef  SIRIUS
        lea     MEM_ERR_PAGE,a0
        movl    #MERR_BASE,d0
        movsl   a0,a5@(0,d0:L)          | page for Memory error registers
        lea     ECC_MEM_PAGE,a0         | ECC Memory Control Registers
        movl    #ECC_MEM_BASE,d0
        movsl   a0,a5@(0,d0:L)
        movw    #0x40,d0
        lea     ECC_MEM_ENA_REG,a5
        movw    d0,a5@                  | write ECC MEM ENABLE REGISTER
#endif  SIRIUS

        movl    #0x800,sp               | set stack pointer
#ifdef	SIRIUS
        movl    #8,d2
#else
	movl	#2,d2
#endif	SIRIUS
        jra     esckey                  | bail out to end
#endif	MJC
9$:
	cmpb	#op_next,d0		| go to next test?
	bne	10$			| if not
	bset	#bit_tst_err,d7		| clear test error flag bit
	bra	40$
10$:
	cmpb	#op_restart,d0		| restart test?
	bne	14$			| if not
	jra	_selftest		| yes, restart selftest
14$:
	cmpb	#op_no_print,d0		| no print option?
	bne	22$
	btst	#bit_no_print,d7	| already set?
	beq	20$			| if not
	bclr	#bit_no_print,d7	| if so, then clear
	bra	22$
20$:	
	bset	#bit_no_print,d7	| then set no print bit	
22$:	
	cmpb	#op_burn_in,d0		| execute burn in test
	bne	30$			| if not
	btst	#bit_burn_in,d7		| already set?
	beq	24$			| if not
	bclr	#bit_burn_in,d7		| if so, then clear
	bra	30$
24$:
	bset	#bit_burn_in,d7		| then set burn in bit
30$:
	cmpb	#op_print_all,d0	| print all errors?
	bne	40$			| if not
	btst	#bit_print_all,d7	| already set?
	beq	34$			| if not
	bset	#bit_print_all,d7	| then set print all errs/no loop
	bra	40$
34$:
	bclr	#bit_print_all,d7	| then clr print all errs/no loop
40$:
	btst	#bit_print_all,d7	| don't loop on error?
	bne	100$			| if yes
	btst	#bit_tst_err,d7		| has this test failed?
	bne	100$			| if not
	movw	#0x3700,sr		| master mode
	movl	sp,a0			| get msp--saved top of loop
	movw	#0x2700,sr		| nonmaster mode
	jra	a0@			| return, top of loop
100$:
	jra	a6@			| next instruction after call
	
|----------------------------------------------------------------------------
|	MESSAGE DISPLAY ROUTINE
|
print$:
	movl	d0, a0			| save contents of d0 
	movl    d1, a1                  | save contents of d1
	movl    d2, a2                  | save contents of d2
	movl    d3, a3                  | save contents of d3
	movl	a0, usp
|
|	Check if the Diagnostic Switch is On
|
	btst	#bit_no_print, d7
	bne     15$                     | if 0, skip printout
|
|	Set Port A flag and save message address in upper bits of a4
|
	bclr	#bit_port_flag, d7	| Set Port flag to A
	movl	a4, d4			| Save message address
|
|	Check next character of message 
|
1$:	clrl    d3                      | clear reg & digit counters
	movl    #FC_SP, d0             
        movc    d0, sfc
        movsb   a4@, d0
	tstb	d0			| Has message been displayed?
	jne	100$			| If not, continue displaying it
|
|	Check if this display was Port A or B
|
	btst	#bit_port_flag, d7      | Port A or B?
	jne	15$			| If this was Port B, we are done

	bset	#bit_port_flag, d7      | Else, Set Port flag to B
	movl	d4, a4			| Restore the message pointer
	jra	1$			| Start displaying message
|
|	Check for registers to be displayed
|
100$:
	cmpb	#0x25, d0		| Is this a '%'?
	jne	125$			| If not, this is a character
|
|	This is a register to be displayed
|
	addql	#1, a4			| waste the %
	movsb	a4@+, d0		| and find register to print
	cmpb	#0x64, d0		| is 'd'??
	jne	2$			| branch to see if 'a'
|
|	This is a data register
|
	movsb	a4@+, d0		| get register #
	andl	#7, d0			| hope is '0'..'3''4'
	movl	usp, a0
	movl	a0, d2
	cmpb	#0, d0			| Is this register d0 ?
	jeq	10$
	movl    a1, d2 
        cmpb    #1, d0                  | Is this register d1 ? 
        jeq     10$
	movl    a2, d2 
        cmpb    #2, d0                  | Is this register d2 ? 
        jeq     10$
        movl    a3, d2                  | Then it is register d3 ?
        jra     10$

|	Check if it is an address register 
|
2$:	cmpb	#0x61, d0		| is 'a'??
	jne	125$			| nope, print char
|
|	This is an address register
|
	movsb	a4@+, d0		| else get register #
	movl	a5, d2
|
|	Determine the contents of the register to be displayed
|
10$:	movl    #FC_SP, d0
        movc    d0, sfc
	movl	d2, d0
	roll	#4, d0			| get next highest digit
	movl	d0, d2			| and save back
	andl	#0xf, d0		| mask it
	movl	d0, a0			| set up byte grab
	addl	#_chardigs, a0		| convert it to an ascii digit
|
|       Send a digit to the terminal
|
	movsb   a0@, d6

	moveq   #FC_MMU, d0             | set up source and dest fc
        movc    d0, sfc
        movc    d0, dfc

	btst    #bit_port_flag, d7	| Port A?
	jne	120$			| If not, print to Port B
|
|	Display to Port A
|
	moveq   #-1, d0                 
#ifdef	M25
11$:	movb	UARTACNTL,d1		| wait for SCC to be ready
#else
11$:    movsb   UARTACNTL, d1
#endif M25
	btst	#TXREADY, d1		| Wait for SCC to be ready
	dbne	d0, 11$			| Loop until ready or timeout

	moveq	#0xf, d0
12$:	dbra	d0, 12$			| Wait for character to be transmitted

#ifdef	M25
	movb	d6, UARTADATA		| Send the character out
#else
	movsb   d6, UARTADATA           | Send the character out
#endif M25
	jra	123$			| else go on printing
| 
|       Display to Port B 
|
120$:
	moveq   #-1, d0
#ifdef  M25
121$:   movb    UARTBCNTL,d1            | wait for SCC to be ready
#else  
121$:   movsb   UARTBCNTL, d1
#endif M25
        btst    #TXREADY, d1            | Wait for SCC to be ready
        dbne    d0, 121$                | Loop until ready or timeout
 
        moveq   #0xf, d0
122$:   dbra    d0, 122$                | Wait for character to be transmitted
 
#ifdef  M25
        movb    d6, UARTBDATA           | Send the character out
#else  
        movsb   d6, UARTBDATA           | Send the character out
#endif M25
 
123$:
        addql   #1, d3                  | bump digit counter
        btst    #3, d3                  | check digit count == 8
        jeq     10$                     | it bit for 8*4 not set , go on
        jra     1$                      | else go on printing
|
|	Send the character to the terminal
|
125$:	movsb   a4@+, d6

	moveq   #FC_MMU, d0             | set up source and dest fc
        movc    d0, sfc
        movc    d0, dfc

        btst    #bit_port_flag, d7      | Port A?
        jne     140$                    | If not, print to Port B
| 
|       Display to Port A 
|
	moveq   #-1, d0
#ifdef	M25
13$:	movb	UARTACNTL,d1		| wait for SCC to be ready
#else
13$:    movsb   UARTACNTL, d1
#endif M25
        btst    #TXREADY, d1            | Wait for SCC to be ready
        dbne    d0, 13$                 | Loop until ready or timeout

	moveq   #0xf, d0
14$:    dbra    d0, 14$

#ifdef	M25
	movb	d6,UARTADATA		| Send the character out
#else
	movsb   d6, UARTADATA           | Send the character out
#endif M25
	jra	143$
| 
|       Display to Port B 
|
140$:
	moveq   #-1, d0
#ifdef  M25
141$:   movb    UARTBCNTL,d1            | wait for SCC to be ready
#else   
141$:   movsb   UARTBCNTL, d1
#endif M25
        btst    #TXREADY, d1            | Wait for SCC to be ready
        dbne    d0, 141$                | Loop until ready or timeout
 
        moveq   #0xf, d0
142$:   dbra    d0, 142$
 
#ifdef  M25
        movb    d6,UARTBDATA            | Send the character out
#else   
        movsb   d6, UARTBDATA           | Send the character out
#endif M25
143$:   jra     1$
|	
|	Message has been displayed.
|	Return to main program.
|
15$:	moveq   #FC_MMU, d0             | set up source and dest fc
        movc    d0, sfc
        movc    d0, dfc
	movl	usp, a0
	movl	a0, d0			| restore the contents of d0
	movl    a1, d1                  | restore the contents of d1
	movl    a2, d2                  | restore the contents of d2
	movl    a3, d3                  | restore the contents of d3
        jmp     a6@                     | and return

|-------------------------------------------------------------------------------
|
|       Initialize UARTs for later tests (uses uartinit table)
|       Set up A as I/O device
|
UARTinit:                               | init uart
        clrl    d0
        lea     uartainit, a0           | set up pram xfer for A

	movsb   ENABLEOFF,d0            | read diag switch
        btst    #DIAGSW,d0              | Diagnostic mode (diag sw = 1)?
        beq     6$                      | if not, do not initialize UART

1$:     movl    #FC_SP, d1              | get next byte from init table
        movc    d1, sfc
        movc    d1, dfc
        movsb   a0@+, d0
	moveq   #FC_MMU, d1
        movc    d1, sfc
        movc    d1, dfc
        cmpb    #0xff, d0               | are we done yet??
        jeq     3$

#ifdef	M25
	movb	d0, UARTACNTL		| stuff
#else
	movsb   d0, UARTACNTL           | stuff
#endif M25

        movl    #0x800, d1              | and wait
2$:     dbra    d1, 2$
        jra     1$
|
|	Set Port B Baud rate to 1200
|
3$:	
        lea     uartbinit, a0           | set up pram xfer for A
 
4$:     movl    #FC_SP, d1              | get next byte from init table
        movc    d1, sfc
        movc    d1, dfc
        movsb   a0@+, d0
        moveq   #FC_MMU, d1
        movc    d1, sfc
        movc    d1, dfc
        cmpb    #0xff, d0               | are we done yet??
        jeq     6$
 
#ifdef  M25
        movb    d0, UARTBCNTL           | stuff
#else   
        movsb   d0, UARTBCNTL           | stuff
#endif M25
 
        movl    #0x800, d1              | and wait
5$:     dbra    d1, 5$
	jra	4$

6$:	jmp     a6@

	.data
	.even
uartainit:
        .word   0x09c0          | Force hardware reset
        .word   0x0446          | X16 clock mode, 1 stop bit/char, even parity
        .word   0x03c0          | Rx is 8 bits/char
        .word   0x05e2          | DTR, Tx is 8 bits/char, RTS
        .word   0x0902          | No vector returned on interrupt      ----
        .word   0x0b55          | Rx Clock = Tx Clock = BR generator = TRxC OUT
        .word   0x0c0e          | Time Constant = 0x000e (9600 Baud)
        .word   0x0d00
        .word   0x0e02          | BR generator comes from Z-SCC's PCLK input
        .word   0x03c1          | Rx is 8 bits/char, Rx is enabled
        .word   0x05ea          | DTR, Tx is 8 bits/char, Tx is enabled, RTS
        .word   0x0e03          | BR comes from PCLK, BR generator is enabled
        .word   0x0030          | Error reset
        .word   0x0030          | Error reset
	.word   0x0010		| external status reset
        .word   0xffff          | Terminator for UART initialization table
uartbinit:
	.word   0x0010          | external status reset
        .word   0x0446          | X16 clock mode, 1 stop bit/char, even parity
        .word   0x03c0          | Rx is 8 bits/char
        .word   0x05e2          | DTR, Tx is 8 bits/char, RTS
        .word   0x0902          | No vector returned on interrupt      ----
        .word   0x0b55          | Rx Clock = Tx Clock = BR generator = TRxC OUT
	.word   0x0c7e          | Time Constant = 0x007e (1200 Baud)
        .word   0x0d00
        .word   0x0e02          | BR generator comes from Z-SCC's PCLK input
        .word   0x03c1          | Rx is 8 bits/char, Rx is enabled
        .word   0x05ea          | DTR, Tx is 8 bits/char, Tx is enabled, RTS
        .word   0x0e03          | BR comes from PCLK, BR generator is enabled
        .word   0x0010          | external status reset
	.word   0x0010          | external status reset
        .word   0xffff          | Terminator for UART initialization table
program_name:
        .asciz  "\015\012Boot PROM Selftest\012"
Test_01_txt:
        .asciz  "\015\012PROM Checksum Test"
Test_02_txt:
	.asciz	"\015\012DVMA Reg Test"
wr_rd_err_txt:
	.asciz	"\015\012 Err 1: Exp 0x%d1, Obs 0x%d0, Xor 0x%d2"
mem_rd_err_txt:
        .asciz  "\015\012 Err 2: Addr 0x%a5, Exp 0x%d1, Obs 0x%d0"
xor_rd_err_txt:
	.asciz  "\015\012 Err 2: Addr 0x%a5, Exp 0x%d1, Obs 0x%d0, Xor 0x%d2"
Test_03_txt:
	.asciz	"\015\012Context Reg Test"
Test_04_txt:
	.asciz	"\015\012Segment Map Wr/Rd Test"
Test_05_txt:
	.asciz	"\015\012Segment Map Address Test" 
Test_06_txt:
	.asciz	"\015\012Page Map Test"
Test_07_txt:
	.asciz	"\015\012Memory Path Data Test"
Test_08_txt:
        .asciz  "\015\012NXM Bus Error Test"
NXM_err_txt:
	.asciz	"\015\012 Err 3: No NXM Bus Error!"
Test_09_txt:
        .asciz  "\015\012Interrupt Test"
int_error_txt:
	.asciz	"\015\012 Err 4: No Level 1 interrupt!"
Test_0A_txt:
	.asciz	"\015\012TOD Clock Interrupt Test"
int_TOD_error_txt:
        .asciz  "\015\012 Err 21: TOD failed to interrupt!"
MMU_access_txt:
        .asciz  "\015\012MMU Access Bit Test"
MMU_accmod_txt:
        .asciz  "\015\012MMU Access/Modify Bit Test"
MMU_invalid_txt:
        .asciz  "\015\012MMU Invalid Page Test"
MMU_protect_txt:
        .asciz  "\015\012MMU Protected Page Test"
access_err_txt:
	.asciz	"\015\012 Err 5: Page Map status, Exp 0x%d1, Obs 0x%d0, Xor 0x%d5"
inv_ac_err_txt:
	.asciz	"\015\012 Err 6: No bus err when accessing invalid page!"
be_status_txt:
        .asciz  "\015\012 Err 7: Bus error reg  Exp 0x%d1, Obs 0x%d0, Xor 0x%d2"
wr_prot_err_txt:
	.asciz	"\015\012 Err 8: No bus error when writing a protected page!" 
#ifndef	SIRIUS
no_par_trap_txt:
	.asciz	"\015\012 Err 9: Bad parity should cause nmi"
par_st_err_txt:
	.ascii	"\015\012 Err 10: Parity error at Addr 0x%a5, Data 0x%d3,"
	.asciz	"\015\012 Status Exp 0x%d1, Obs 0x%d0"
#endif	SIRIUS
mem_sizing_txt:
	.asciz	"\015\012Sizing Memory"
#ifdef SIRIUS
Test_ecc_txt:
	.asciz	"\015\012ECC Error Tests"
#ifdef	NOT_USEFUL
Test_0D_txt:
	.asciz  "\015\012Cache Data Wr/Rd Test"
Test_0F_txt:
        .asciz  "\015\012Cache Tags Wr/Rd Test"
#endif	NOT_USEFUL
Test_0E_txt:
	.asciz  "\015\012Cache Data 3 Pat Test"
Test_10_txt:
	.asciz  "\015\012Cache Tags 3 Pat Test"
mem_invalid_txt:
	.asciz  "\015\012 Err 17: Invalid memory size \015\012"
syn_err_txt:
	.ascii  "\015\012 Err 18: UE or CE error syndrome reg 0x%d0"
	.asciz  " Mem err reg 0x%d1"
|mem_no_bd_txt:
|	.asciz  "\015\012 Err 19: No memory boards!"
ce_err_txt:
	.asciz  "\015\012 Err 11: Syndrome error, Exp 0x%d1 Obs 0x%d0"
ecc_1_txt:
	.asciz  "\015\012 Err 20: Data correction error: Exp 0x%d3 Obs 0x%d0"
unex_ecc_txt:
	.asciz  "\015\012 Err 15: Unexp'd ECC err trap, pc 0x%d0, mem_err 0x%d5"
#else
Test_0C_txt:
	.asciz  "\015\012Parity Test"
mem_par_err_txt:
	.ascii  "\015\012 Err 11: Parity Error at address 0x%a5"
        .asciz  "\015\012 Exp 0x%d1, Obs 0x%d0, Xor 0x%d2"
continue_txt:
	.asciz  "\015\012 Continuing test beyond 0x%a5 ...\015\012"
unex_pe_txt:
        .asciz  "\015\012 Err 15: Unexp'd parity err trap, pc 0x%d0, mem_err 0x%d5"
#endif SIRIUS
Test_11_txt:
	.asciz	"\015\012Memory Test (testing %d3 Mbytes)"
nmi_ques_int_txt:
	.asciz	"\015\012 Err 12: NMI int with bad status, obs 0x%d0"
unex_trap_txt:
	.asciz	"\015\012 Err 13: Unexp'd trap/vector, pc 0x%d0"
trap_txt:
	.asciz	" vector offset 0x%d0"
unex_be_txt:
	.asciz	"\015\012 Err 14: Unexp'd bus err, pc 0x%d0"
access_txt:
	.asciz	" access addr 0x%d0"
berr_txt:
	.asciz	" bus err reg 0x%d0"
test_pass_txt:
	.asciz  "\015\012\012Selftest passed.\015\012"
test_fail_txt:
	.asciz	"\015\012\012Selftest failed.\015\012"
chksm_err_txt:
	.asciz	"\015\012 Err 16: Checksum error: Exp 0x%d0, Obs 0x%d1"
m_op_tests:
	.asciz	" (e for echo mode) "
echo_msg:
	.asciz	"\015\012Echo mode: Output will echo to Video!\015\012"
