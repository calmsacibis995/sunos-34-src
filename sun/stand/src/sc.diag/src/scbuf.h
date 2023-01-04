

/* ======================================================================
   Author: Peter Costello
   Date :  April 12, 1983
   Modifications:
   Purpose: This file contains standard constants and address offsets 
	for use of the Sun-2 color board.
   Note: All registers are on 4K-byte boundaries. This allows the OS
	full "RWXRWX" protection on each register.
   Bugs:
   ====================================================================== */
/*      @(#)scbuf.h 1.1 9/25/86 Copyright Sun Microsystems, Inc. */

typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned int   uint;

#define TRUE 1
#define True 1
#define true 1
#define FALSE 0
#define False 0
#define false 0

extern int SCBase;		/* start address in current virtual address */
				/* space, of color buffer.  It is initialized */
				/* with ioctl() on /dev/scx */
extern int SCWidth;		/* 1152 or 1024 */
extern int SCHeight;		/* 909 or 1024 */
extern int SCRop;		/* Base of RasterOp chip using */

/* extern int SCconfig;		*/
/* extern char *SCDeviceName;	*/
/* extern int SCFile;		*/

/* Word-Mode Memory. MByte 0. */
#define SC_Mem0		(ushort*)(SCBase + 0x000000) 
#define SC_Mem1		(ushort*)(SCBase + 0x020000) 
#define SC_Mem2		(ushort*)(SCBase + 0x040000) 
#define SC_Mem3		(ushort*)(SCBase + 0x060000) 
#define SC_Mem4		(ushort*)(SCBase + 0x080000) 
#define SC_Mem5		(ushort*)(SCBase + 0x0A0000) 
#define SC_Mem6		(ushort*)(SCBase + 0x0C0000) 
#define SC_Mem7		(ushort*)(SCBase + 0x0E0000) 
#define SC_Mem8		(ushort*)(SCBase + 0x100000) 

#define SC_Meml0	(uint  *)(SCBase + 0x000000) 
#define SC_Meml1	(uint  *)(SCBase + 0x020000) 
#define SC_Meml2	(uint  *)(SCBase + 0x040000) 
#define SC_Meml3	(uint  *)(SCBase + 0x060000) 
#define SC_Meml4	(uint  *)(SCBase + 0x080000) 
#define SC_Meml5	(uint  *)(SCBase + 0x0A0000) 
#define SC_Meml6	(uint  *)(SCBase + 0x0C0000) 
#define SC_Meml7	(uint  *)(SCBase + 0x0E0000) 
#define SC_Meml8	(uint  *)(SCBase + 0x100000) 

/* Pixel-Mode Memory. MByte 1. */
#define SC_Pix		(uchar*) (SCBase + 0x100000)
#define SC_Pixt		(uchar*) (SCBase + 0x200000)
#define SC_Pixl		(uint *) (SCBase + 0x100000)
#define SC_Pixlt	(uint *) (SCBase + 0x200000)

/* Word-Mode RasterOP. MByte 2. */
#define SC_RMem0	(ushort*)(SCBase + 0x200000) 
#define SC_RMem1	(ushort*)(SCBase + 0x220000) 
#define SC_RMem2	(ushort*)(SCBase + 0x240000) 
#define SC_RMem3	(ushort*)(SCBase + 0x260000) 
#define SC_RMem4	(ushort*)(SCBase + 0x280000) 
#define SC_RMem5	(ushort*)(SCBase + 0x2A0000) 
#define SC_RMem6	(ushort*)(SCBase + 0x2C0000) 
#define SC_RMem7	(ushort*)(SCBase + 0x2E0000) 
#define SC_RMem8	(ushort*)(SCBase + 0x300000) 

#define SC_RMeml0	(uint  *)(SCBase + 0x200000) 
#define SC_RMeml1	(uint  *)(SCBase + 0x220000) 
#define SC_RMeml2	(uint  *)(SCBase + 0x240000) 
#define SC_RMeml3	(uint  *)(SCBase + 0x260000) 
#define SC_RMeml4	(uint  *)(SCBase + 0x280000) 
#define SC_RMeml5	(uint  *)(SCBase + 0x2A0000) 
#define SC_RMeml6	(uint  *)(SCBase + 0x2C0000) 
#define SC_RMeml7	(uint  *)(SCBase + 0x2E0000) 
#define SC_RMeml8	(uint  *)(SCBase + 0x300000) 

/* Pixel-Mode RasterOp. MByte 2. */
#define SC_RPix		(uchar*) (SCBase + 0x200000)
#define SC_RPixt	(uchar*) (SCBase + 0x300000)
#define SC_RPixl	(uint *) (SCBase + 0x200000)
#define SC_RPixlt	(uint *) (SCBase + 0x300000)

/* Control Register Base. MByte 3. */
#define SC_Ctrl		(SCBase + 0x300000)

/* Define Control Register Addresses. */
#define SC_Stat		*(ushort*)(SC_Ctrl + 0x009000)
#define SC_Mask		*( uchar*)(SC_Ctrl + 0x00A001)
#define SC_WPan		*(ushort*)(SC_Ctrl + 0x00B000) 	/* Word Pan */
#define SC_Zoom		*( uchar*)(SC_Ctrl + 0x00C001) 	/* Zoom */
#define SC_PPan		*( uchar*)(SC_Ctrl + 0x00D001) 	/* Pix Offset */
#define SC_VZoom	*( uchar*)(SC_Ctrl + 0x00E001) 	/* Var Zoom Reg */
#define SC_IVect	*( uchar*)(SC_Ctrl + 0x00F001) 	/* Interrupt Vector */

#define SCW_Stat	*(ushort*)(SC_Ctrl + 0x009000)
#define SCW_Mask	*(ushort*)(SC_Ctrl + 0x00A000)
#define SCW_WPan	*(ushort*)(SC_Ctrl + 0x00B000) 	/* Word Pan */
#define SCW_Zoom	*(ushort*)(SC_Ctrl + 0x00C000) 	/* Zoom */
#define SCW_PPan	*(ushort*)(SC_Ctrl + 0x00D000) 	/* Pix Offset */
#define SCW_VZoom	*(ushort*)(SC_Ctrl + 0x00E000) 	/* Var Zoom Reg */
#define SCW_IVect	*(ushort*)(SC_Ctrl + 0x00F000) 	/* Interrupt Vector */

/* Define Color Map Base Addresses. */
#define SC_Red_Cmap	 (ushort *)(SC_Ctrl + 0x010000)
#define SC_Grn_Cmap	 (ushort *)(SC_Ctrl + 0x010200)
#define SC_Blu_Cmap	 (ushort *)(SC_Ctrl + 0x010400)

/* Define to select one or all ROPC and one or all Memory Planes */
#define Set_Ropc(x)	SCRop = SC_Ctrl + (0x1000*x); 	\
			if (x<8) {SC_Mask=(1<<x);} else {SC_Mask=0xFF;}

/* Define offsets for Registers in ROPC chips */
#define SC_Dst		*(ushort*)(SCRop + 0x0000)
#define SC_Src1		*(ushort*)(SCRop + 0x0002)
#define SC_Src2		*(ushort*)(SCRop + 0x0004)
#define SC_Pat		*(ushort*)(SCRop + 0x0006)
#define SC_Msk2		*(ushort*)(SCRop + 0x0008)
#define SC_Msk1		*(ushort*)(SCRop + 0x000A)
#define SC_SftVal	*(ushort*)(SCRop + 0x000C)	/* SRC Shift Amt */
#define SC_Func		*(ushort*)(SCRop + 0x000E)
#define SC_Width	*(ushort*)(SCRop + 0x0010)
#define SC_OpCnt	*(ushort*)(SCRop + 0x0012)	/* Counts down */
#define SC_Fout		*(ushort*)(SCRop + 0x0014)	/* ROPC output */
#define SC_Ldst		*(ushort*)(SCRop + 0x0016)
#define SC_Lsrc		*(ushort*)(SCRop + 0x0018)
#define SC_Flag		*(ushort*)(SCRop + 0x001E)

/* Define Data Bits in Copy and Status Register */
#define VEnable		0x0001			/* Video Enable */
#define UpECmap		0x0002			/* Update ECL Cmap */
#define Inten		0x0004			/* Enable Interrupts */
#define RopMod0		0x0018			/* RasterOp Mode Bit 0 */
#define RopMod1		0x0010			/* RasterOp Mode Bit 1 */
#define RopMod2		0x0020			/* RasterOp Mode Bit 2 */
#define IReq		0x0040			/* Int pending. Read Only */
#define VBlank		0x0080			/* Blanking. Read Only */
#define Res_1k1k	0x0100			/* Resolution = 1k x 1k */

/* Define some useful operations */
#define Enable_All_Planes	SCW_Mask = 0xFF
#define Init_Stat		SC_Stat = VEnable
#define SC_Retrace		(SC_Stat & VBlank)
#define SC_IReq			(SC_Stat & IReq)
#define Acquire_Cmap		SC_Stat &= ~UpECmap
#define Release_Cmap	 	SC_Stat |= UpECmap 
#define Rop_Mode(x)		SC_Stat = (x << 3) + (SC_Stat & 0x07)
#define ROP_Mode(x)		SC_Stat = (x << 3) + (SC_Stat & 0x07)


/* The following are function register encodings */
# define SC_copy           (uchar)0xCC  /* Copy data reg to Frame buffer */
# define SC_copy_invert    (uchar)0x33  /* Copy inverted data reg to FB  */
# define SC_wr_creg        (uchar)0xF0  /* Copy color reg to Frame buffer */
# define SC_wr_mask        (uchar)0xF0  /* Copy mask to Frame buffer */
# define SC_inv_wr_creg    (uchar)0x0F  /* Copy inverted Creg to FB */
# define SC_inv_wr_mask    (uchar)0x0F  /* Copy inverted Mask to FB */
# define SC_ram_invert     (uchar)0x55  /* 'Invert' color in Frame buffer */
# define SC_cr_and_dr      (uchar)0xC0  /* Bitwise and of color and data regs */
# define SC_clear	   (uchar)0x00  /* Clear frame buffer */
# define SC_cr_xor_fb	   (uchar)0x5A  /* Xor frame buffer data and Creg */
# define SC_set		   (uchar)0xFF  /* Set bit in frame buffer */


/* ===============================================================
   Global Variable Definitions.
   These two arrays hold the default colors in the colormap.
   =============================================================== */

extern ushort
    sc_red[256],	/* Default reds   */
    sc_grn[256],	/* Default greens */
    sc_blu[256];	/* Default blues */
