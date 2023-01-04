|
|	Vector Table for the Sun Rom Monitor
|
|	This is the first thing in the PROM.  It provides the RESET
|	vector which starts everything on power-up, as well as assorted
|	information about where to find things in the ROMs and in low
|	memory.
|
| 	@(#)romvec.s 1.15 84/08/15 Copyright (c) 1984 by Sun Microsystems, Inc.
|

#include "assym.h"
#include "../../sys/sun/fbio.h"

 .long	INITSP		| Initial SSP for hardware RESET
 .long	_hardreset	| Initial PC  for hardware RESET
| Monitor and hardware revision and identification
 .long	_monrev		| Revision level of monitor & hardware
 .long	bootaddr	| Addr of addr of boot parameters
 .long	g_memorysize	| Physical onboard memory size
| Monitor-managed single-character input and output
 .long	_getchar	| Get char from cur input source
 .long	_putchar	| Put char to current output sink
 .long	_mayget		| Maybe get char from current input source
 .long	_mayput		| Maybe put char to current output sink
 .long	g_echo		| Should getchar echo its input?
 .long	g_insource	| Input source selector
 .long	g_outsink	| Output sink selector
| Monitor-managed keyboard input (scanned by monitor refresh routine)
 .long	_getkey		| Get next translated key if one exists
 .long	_initgetkey	| Initialize before first getkey
 .long	g_translation	| Up/down keyboard translation selector
 .long	g_keybid	| Up/down keyboard ID byte
 .long	0		| WAS: Address of current keyboard defn struct
 .long	0		| WAS: Address of current shift->table mapper
 .long	g_keybuf	| Up/down keycode buffer
 .long	_monrev		| New location of monitor revision information
| Monitor-managed framebuffer output and terminal emulation
 .long	_fwritechar	| Write a character to FB "terminal"
 .long	fbaddr		| Address of frame buffer
 .long	g_font		| Address of current font definition
 .long	_fwritestr	| Write a string to FB terminal - faster
| Entry point to request a re-boot of the system
 .long 	_boot_me	| Boot with the specified parameter (like "b" command.)
| Monitor-managed line input and parsing
 .long	g_linebuf	| The line input buffer
 .long	g_lineptr	| Current pointer into g_linebuf
 .long	g_linesize	| Total length of line in g_linebuf
 .long	_getline	| Fill g_linebuf from current input source
 .long	_getone		| Get next char from g_linebuf
 .long	_peekchar	| Peek at next char without reading it
 .long	g_fbthere	| Is frame buffer physically there?
 .long	_getnum		| Get next numerics and xlate to binary
| Monitor-managed phrase output to current output sink
| The next one used to be _message, and there is a small incompatability
| if you now print % characters thru it.  Most people had their own printf
| and just used putchar(), so it's not a problem.
 .long	_printf		| Print a null-terminated string
 .long	_printhex	| Print N digits of a longword in hex
| Assorted
 .long	g_leds		| RAM copy of LED register value
 .long	_set_leds	| Sets LED register and RAM copy to argument value
| Refresh related information
 .long	_nmi		| Address that oughta be in level 7 vector
 .long	_abortent	| Monitor entry point from keyboard abort
 .long	g_nmiclock	| Refresh routines's millisecond count
| Assorted things added after the first glut
#ifdef S2COLOR
 .long	g_fbtype	| Which type of frame buffer do we have at runtime?
#else  S2COLOR
 .long	fbtype		| Which type of frame buffer do we support?
#endif S2COLOR
 .long	0		| WAS: Address of keyboard exception routine pointer
| Monitor internal interface information
 .long	gp		| Pointer to global data structure
 .long	g_keybzscc	| Pointer to current keyboard's zscc
| Assorted things added for Rev D
 .long	g_keyrinit	| ms to wait before repeating a held key
 .long	g_keyrtick	| ms to wait between repetitions ditto
 .long	0		| WAS: ptr to table of strings gen'd by keyboard
 .long	g_resetaddr	| vector address for watchdog resets
 .long	g_resetmap	| page map entry for watchdog resets
 .long	_exit_to_mon	| Exit-to-monitor entry point

|
| FIXME.  This points to the boot parameters.  They are currently at 0x2000
| because old programs assume them to be there.  We can move them to
| globram or elsewhere once everyone is indirecting to find them.
|
bootaddr:
 .long	0x2000		| Temporary home of boot parameters

#ifdef S2COLOR
| Type of frame buffer is defined at run time.
#else
|
| Define type of frame buffer we support.
|
fbtype:
#ifdef S2FRAMEBUF
 .long	FBTYPE_SUN2BW		| Sun-2 black & white frame buffer
#else
#ifdef FRAMEBUF
 .long	FBTYPE_SUN1BW		| Sun-1 black & white frame buffer
#else
 .long	FBTYPE_SUN1BW		| No frame buffer support -- fake it.
#endif
#endif
#endif
