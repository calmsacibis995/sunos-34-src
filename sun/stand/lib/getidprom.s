	.data
	.asciz	"@(#)getidprom.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

| Stolen from the ROM Monitor, version:
|	 @(#)getidprom.s 1.2 83/12/29 Copyr 1983 Sun Micro

|	Copyright (c) 1983 by Sun Microsystems, Inc.

| FIXME, crude hackery here to avoid include dependencies.
FC_MAP = 3
BYTESPERPG = 0x800
IDPROMOFF = 8;

|
| getidprom(addr, size)
|
| Read back <size> bytes of the ID prom and store them at <addr>.
| Typical use:  getidprom(&idprom_struct, sizeof(idprom_struct));
|
	.globl	_Getidprom
_Getidprom:
	movl	sp@(4),a0	| address to move ID prom bytes to
	movl	sp@(8),d1	| How many bytes to move
	movl	d2,sp@-		| save a reg
	movc	sfc,d0		| save source func code
	movl	#FC_MAP,d2
	movc	d2,sfc		| set space 3
	lea	IDPROMOFF,a1	| select id prom
	jra	2$		| Enter loop at bottom as usual for dbra
1$:	movsb	a1@,d2		| get a byte
	movb	d2,a0@+		| save it
	addw	#BYTESPERPG,a1	| address next byte (in next page)
2$:	dbra	d1,1$		| and loop
	movc	d0,sfc		| restore sfc
	movl	sp@+,d2		| restore d2
	rts
