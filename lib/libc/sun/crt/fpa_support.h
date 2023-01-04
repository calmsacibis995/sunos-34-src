/*	@(#)fpa_support.h 1.1 86/09/24 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#define FPABASE 0xe0000000
#define STATIC static

extern double fpa_transcendental( ) ;
extern double fpa_tolong( ) ;

union   singlekluge
	{
	float x ;
	long  i[1] ;
	} ;

union   doublekluge
	{
	double x ;
	long   i[2] ;
	} ;

struct inststruct		/* Instruction format. */ 
	{ /* Instruction field, including valid bit. */
	unsigned short valid : 1 ;
	unsigned hijunk : 2 ;
	unsigned op : 6 ;
	unsigned reg1 : 4 ;
	unsigned prec : 1 ;
	unsigned lojunk : 2 ; 
	} ;

struct fpa_access		/* Record for one fpa access. */
	{
	union 
		{
		short kint ;
		struct inststruct kis ;
		} kluge ;
	long	data ;		/* Data field - 32 bits. */
	} ;

struct fpa_inst			/* Record for one fpa instruction. */
	{
	struct fpa_access a[2] ; 
	} ;
