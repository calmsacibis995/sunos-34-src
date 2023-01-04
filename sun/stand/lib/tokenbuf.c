/*
 *	this is the default buffer to be used to parsing tokens.
 *	if you gets(buffer) and tokenparse(buffer), then tokenbuf
 *	is the private buffer copied to to eat up.  You may also
 *	do gets(tokenbuf) and tokenparse(tokenbuf) if the buffer
 *	doesnt matter to you, and you want to save space.
 *	if you want a smaller tokenbuf, just declare something
 *	as tokenbuf (and has some room, no checks made!!)
 *
 *	@(#)tokenbuf.c 1.1 9/25/86 Copyright Sun Micro
 */
char tokenbuf[256];
