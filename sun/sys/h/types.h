/*	@(#)types.h 1.1 86/09/25 SMI; from UCB 4.11 83/07/01	*/

/*
 * Basic system types and major/minor device constructing/busting macros.
 */
#ifndef _TYPES_
#define	_TYPES_

#ifndef KERNEL
#include	<sys/sysmacros.h>
#else
#include	"../h/sysmacros.h"
#endif

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* System V compatibility */
typedef	unsigned int	uint;		/* System V compatibility */

#ifdef vax
typedef	struct	_physadr { int r[1]; } *physadr;
typedef	struct	label_t	{
	int	val[14];
} label_t;
#endif
#ifdef mc68000
typedef	struct	_physadr { short r[1]; } *physadr;
typedef	struct	label_t	{
	int	val[13];
} label_t;
#endif
typedef	struct	_quad { long val[2]; } quad;
typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	u_long	ino_t;
typedef	long	swblk_t;
typedef	int	size_t;
typedef	long	time_t;
typedef	short	dev_t;
typedef	int	off_t;
typedef long	key_t;

typedef	struct	fd_set { int fds_bits[1]; } fd_set;

#endif
