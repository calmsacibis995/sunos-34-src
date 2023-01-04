/*	@(#)fbio.h 1.1 84/12/20 SMI	*/

/*
 * Frame buffer descriptor.
 * Returned by FBIOGTYPE ioctl on frame buffer devices.
 */
#ifndef ASM
struct	fbtype {
	int	fb_type;	/* as defined below */
	int	fb_height;	/* in pixels */
	int	fb_width;	/* in pixels */
	int	fb_depth;	/* bits per pixel */
	int	fb_cmsize;	/* size of color map (entries) */
	int	fb_size;	/* total size in bytes */
};
#endif ASM

#define	FBTYPE_SUN1BW		0
#define	FBTYPE_SUN1COLOR	1
#define	FBTYPE_SUN2BW		2
#define	FBTYPE_SUN2COLOR	3
#define	FBTYPE_SUN3BW		4		/* rserved for future Sun use */
#define	FBTYPE_SUN3COLOR	5		/* rserved for future Sun use */
#define	FBTYPE_SUN4BW		6		/* rserved for future Sun use */
#define	FBTYPE_SUN4COLOR	7		/* rserved for future Sun use */
#define	FBTYPE_NOTSUN1		8		/* reserved for customer */
#define	FBTYPE_NOTSUN2		9		/* reserved for customer */
#define	FBTYPE_NOTSUN3		10		/* reserved for customer */

#define	FBTYPE_LASTPLUSONE	11		/* max number of fbs
						   (change as add) */

#define	FBIOGTYPE _IOR(F, 0, struct fbtype)

#ifdef  KERNEL
struct  fbpixrect {
	struct  pixrect *fbpr_pixrect;  /* Pixrect of dev returned here */
};
 
#define FBIOGPIXRECT _IOWR(F, 1, struct fbpixrect)
#endif

