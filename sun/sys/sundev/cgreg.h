/*	@(#)cgreg.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Register definitions for Sun Color Board
 */
#define CGSIZE	(16*1024)	/* 16K of address space */

# define GR_bd_sel   CGXBase	/* Select Color Board */

# define GR_x_select  0x0800    /* Access a column in the frame buffer */
# define GR_y_select  0x0000    /* Access a row in the frame buffer */
# define GR_y_fudge   0x0200    /* Bit 9 not used at all */ 
# define GR_update    0x2000    /* Update frame buffer if this bit set */
# define GR_x_rhaddr  0x1b80    /* Location to read X address bits A9-A8. 
				   Data put into D1-D0. */
# define GR_x_rladdr  0x1b00    /* Location to read X address bits A7-A0.
				   Data put into D7-D0. */
# define GR_y_rhaddr  0x1bc0    /* Location to read Y address bits A9-A8. */
# define GR_y_rladdr  0x1b40    /* Location to read Y address bits A7-A0. */


# define GR_set0      0x0000    /* Address Register pair 0. */
# define GR_set1      0x0400    /* Address Register pair 1. */


# define GR_red_cmap  0x1000    /* Address to select Red Color Map */
# define GR_grn_cmap  0x1100    /* Addr for Green Color Map */
# define GR_blu_cmap  0x1200    /* Addr for Blue Color Map */

# define GR_sr_select 0x1800    /* Addr to select status register */
# define GR_cr_select 0x1900    /* Addr to select mask (color) register */
# define GR_fr_select 0x1a00    /* Addr to select function register */


/* The following are pointers to the mask(color), status, and function regs. */

# define GR_creg      (u_char *)(GR_bd_sel + GR_cr_select)
# define GR_mask      (u_char *)(GR_bd_sel + GR_cr_select)
# define GR_sreg      (u_char *)(GR_bd_sel + GR_sr_select)
# define GR_freg      (u_char *)(GR_bd_sel + GR_fr_select)


/* These assignments are for bits in the Status Register */
# define GRW0_cplane 0x00      /* Select CMap Plane number zero for R/W */
# define GRW1_cplane 0x01      /* Select CMap Plane number one for R/W */
# define GRW2_cplane 0x02      /* Select CMap Plane number two for R/W */
# define GRW3_cplane 0x03      /* Select CMap Plane number three for R/W */

# define GRV0_cplane 0x04      /* Select CMap Plane number zero for video */
# define GRV1_cplane 0x05      /* Select CMap Plane number one for video */
# define GRV2_cplane 0x06      /* Select CMap Plane number two for video */
# define GRV3_cplane 0x07      /* Select CMap Plane number three for video */

# define GR_inten    0x10      /* Enable Interrupt to start at start
                                  of next vertical retrace. Must clear bit to
			          clear interrupts. */

# define GR_paint    0x20      /* Enable Writing five pixels in parallel */
# define GR_disp_on  0x40      /* Enable Video Display */

# define GR_vretrace 0x80      /* Unused on write. On read, true if monitor in
			   	  vertical retrace. */

/* This define returns true if the board is in vertical retrace */
# define GR_retrace  (*GR_sreg & GR_vretrace)

/* The following are function register encodings */
# define GR_copy           0xCC  /* Copy data reg to Frame buffer */
# define GR_copy_invert    0x33  /* Copy inverted data reg to FB  */
# define GR_wr_creg        0xF0  /* Copy color reg to Frame buffer */
# define GR_wr_mask        0xF0  /* Copy mask to Frame buffer */
# define GRinv_wr_creg     0x0F  /* Copy inverted Creg to FB */
# define GRinv_wr_mask     0x0F  /* Copy inverted Mask to FB */
# define GR_ram_invert     0x55  /* 'Invert' color in Frame buffer */
# define GR_cr_and_dr      0xC0  /* Bitwise and of color and data regs */
# define GR_clear	   0x00  /* Clear frame buffer */
# define GR_cr_xor_fb	   0x5A  /* Xor frame buffer data and Creg */
