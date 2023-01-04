/*	@(#)bootparam.h 1.5 83/09/16 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Structure set up by boot command to pass arguments to program booted.
 */
struct bootparam {
	char		*bp_argv[8];	/* string arguments */
	char		bp_strings[100];/* string table for string arguments */
	char		bp_dev[2];	/* device name */
	int		bp_ctlr;	/* controller # */
	int		bp_unit;	/* unit # */
	int		bp_part;	/* partition/file # */
	char		*bp_name;	/* file name, points into bp_strings */
	struct boottab	*bp_boottab;	/* Points to table entry for dev */
};


/*
 * This table entry describes a device.  It exists in the PROM; a
 * pointer to it is passed in bootparam.  It can be used to locate
 * PROM subroutines for opening, reading, and writing the device.
 * NOTE: When using this interface, only ONE device can be open at once.
 * You can't open a tape and a disk.  Sorry.
 */
struct boottab {
	char	b_dev[2];		/* Two char name of dev */
	int	(*b_probe)();		/* probe() --> -1 or found ctlr # */
	int	(*b_boot)();		/* boot(bp) --> -1 or start address */
	int	(*b_open)();		/* open(iobp) --> -1 or 0 */
	int	(*b_close)();		/* close(iobp) --> -1 or 0 */
	int	(*b_strategy)();	/* strategy(iobp,rw) --> -1 or 0 */
	char	*b_desc;		/* Printable string describing dev */
};
