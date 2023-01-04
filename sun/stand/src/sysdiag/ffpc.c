#ifndef lint
static char	sccsid[] = "@(#)ffpc.c 1.1 9/25/86 Copyright Sun Micro";
#endif
/*
 *	Support routines used by the sky diagnostic ffpusr.f
 *	Copyright (c) 1983,1984 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sundev/skyreg.h>

int		skyfd;
char		* devname;
static char	* baseaddr;

int
hxtow_( str, jerr, slen )
register char	*str;
register int	*jerr, slen;
{
	register char	c;
	register	val = 0;

while( slen-- ){
		switch( c = *str++){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    val = (val<<4) + c - '0';
		    break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		    val = (val<<4) + c - 'a' + 10;
		    break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		    val = (val<<4) + c - 'A' + 10;
		    break;
		default:
		    printf("nasty character %c (octal %o) encountered\n", c, c);
		    return val;
		}
	}
	return val;
}

wtohx_( str, slen, vp )
char	*str;
int	slen;
u_short	*vp;
{
	sprintf( str, "%*x", slen, *vp );
}

outw_( reg, data )
short	*reg, *data;
{
    *(short *)(baseaddr + *reg) = *data;
}

inw_( reg, data )
short	*reg, *data;
{
    *data = *(short *)(baseaddr + *reg);
}

outl_( reg, data )
short	*reg;
long	*data;
{
    *(long *)(baseaddr + *reg) = *data;
}

inl_( reg, data )
short	*reg;
long	*data;
{
    *data = *(long *)(baseaddr + *reg);
}

int
iandw_( p, q )
short	*p, *q;
{
    return *p & *q;
}

int 
ixorl_( p, q )
long	*p, *q;
{
    return *p ^ *q;
}

ltohx_( str, slen, vp )
char	*str;
int	slen;
long	*vp;
{
	sprintf( str, "%*x", slen, *vp );
}

int
hxtol_( str, jerr, slen )
char	*str;
int	*jerr, slen;
{
	return hxtow_( str, jerr, slen );
}

/*	systype returns 0 for a Multibus (TM Intel) or 1 for a VMEbus system */
systype_( p )
int	*p;
{
    *p = ((gethostid() & 0xff000000) == 0x02000000);
    return;
}	

/*	slumber sleeps to let other processes run a while */
slumber_( )
{
    sleep(5);
    return;
}	


    /*
     * try to map the sky board into our address space.
     */
mapsky_( p )
int		*p;
{
int pagesize = getpagesize();
extern char	*valloc();

if ((gethostid() & 0xff000000) == 0x02000000) {
    devname = "/dev/vme16";
} else {
    devname = "/dev/mbio";
}
#ifdef DEBUG
    if ((skyfd = open(devname, O_RDWR                ))>0){
	printf("ffpusr: %s already initialized\n", devname);
	close(skyfd);
    }
#endif DEBUG
    if ((baseaddr = valloc( pagesize ))==NULL){
	printf("ffpusr: Can't alloc space for sky\n");
	*p = -1; /* RETURN CODE < 0 FOR ERROR */
	return;
    }
    if ( (*p=(skyfd = open( devname, O_RDWR|O_NDELAY))) < 0 ) {
	printf("ffpusr: Can't open %s\n", devname);
	*p = 0;	/* RETURN CODE 0 = CAN'T OPEN DEVICE */
	return;
    } else {
#ifdef DEBUG
	printf("baseaddr = %x\n",baseaddr);
#endif DEBUG
	if ((gethostid() & 0xff000000) == 0x02000000) {
	    /* VME-bus SYSTEM */
	    if ( mmap( baseaddr, pagesize, PROT_READ|PROT_WRITE,
	        MAP_SHARED, skyfd, 0x8800 )<0 ){
	        printf("ffpusr: mmap failed\n");
	        *p = -1; /* RETURN CODE < 0 FOR ERROR */
	        return;
	    }
	} else {
	    /* MULTIBUS SYSTEM */
	    if ( mmap( baseaddr, pagesize, PROT_READ|PROT_WRITE,
	        MAP_SHARED, skyfd, 0x2000 )<0 ){
	        printf("ffpusr: mmap failed\n");
	        *p = -1; /* RETURN CODE < 0 FOR ERROR */
	        return;
	    }
	}
    }
#ifdef DEBUG
    printf("Status register: %x\n", *(u_short *)(baseaddr+2));
#endif DEBUG
}
