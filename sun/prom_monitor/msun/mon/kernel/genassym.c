/*
 * @(#)genassym.c 1.28.1.1 85/02/21 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This module generates "assym.h" which contains all ".h" values
 * needed by assembler code.  These values are automatically generated
 * from the original ".h" files and automatically keep track of 
 * structure rearrangements, etc.  However, the first time you use
 * such a symbol, you have to add the printf() for it to this module.
 */

#include "../h/s2addrs.h"
#include "../h/s2map.h"
#include "../h/sunmon.h"
#include "../h/globram.h"
#include "../h/am9513.h"
#include "../h/suntimer.h"
#include "../h/enable.h"
#include "../h/m68vectors.h"
#include "../h/zsreg.h"
#include "../h/sunromvec.h"
#include "../h/diag.h"
#include "../h/dpy.h"
#include "../h/am8068.h"
#include "../h/montrap.h"
#include "../../usr.lib/libpixrect/memreg.h"
#include "../../usr.lib/libpixrect/cg2reg.h"

/*
 * These unions are used to define page map entries and enable register
 * values, and get at their hex representations.
 */
union longmap {
	long	longval;
	struct pgmapent pgmapent;
};


union shortena {
	short	shortval;
	struct enablereg enablereg;
};

main()
{
	/*
	 * Declare assorted registers that we're interested in.
	 */
	union longmap mapper;
	union shortena enabler;
	struct intstack *ip = 0;	/* Assume structs start at 0 */
	struct zscc_device *zp = 0;
	struct am9513_device *clkp = 0;
	struct deschip *dp = 0;
	struct monintstack *mp = 0;
	struct cg2fb *cg2 = (struct cg2fb *)
		(VME_COLOR_PHYS + sizeof(struct cg2memfb));

	/*
	 * Fields from s2addrs.h
	 */
	printf("#define SERIAL0_BASE 0x%x\n", SERIAL0_BASE);
	printf("#define PARALLEL_BASE 0x%x\n", PARALLEL_BASE);
	printf("#define TIMER_BASE 0x%x\n", TIMER_BASE);
	printf("#define PROM_BASE 0x%x\n", PROM_BASE);
	printf("#define KEYB_CONTROL 0x%x\n", &KEYBMOUSE_BASE[1].zscc_control);
	printf("#define KEYB_DATA 0x%x\n", &KEYBMOUSE_BASE[1].zscc_data);

	/*
	 * Fields from s2map.h
	 */
	printf("\n");
	printf("#define NUMCONTEXTS %d\n", NUMCONTEXTS);
	printf("#define NUMPMEGS %d\n", NUMPMEGS);
	printf("#define PGSPERSEG %d\n", PGSPERSEG);
	printf("#define BYTESPERPG %d\n", BYTESPERPG);
	printf("#define BYTES_PG_SHIFT %d\n", BYTES_PG_SHIFT);
	printf("#define ADRSPC_SIZE 0x%x\n", ADRSPC_SIZE);
	printf("#define	LOWMASK 0x%x\n", LOWMASK);
	printf("#define PMAPOFF 0x%x\n", PMAPOFF);
	printf("#define SMAPOFF 0x%x\n", SMAPOFF);
	printf("#define	IDPROMOFF 0x%x\n", IDPROMOFF);
	printf("#define CONTEXTOFF 0x%x\n", CONTEXTOFF);
	printf("#define USERCONTEXTOFF 0x%x\n", USERCONTEXTOFF);
	printf("#define SUPCONTEXTOFF 0x%x\n", SUPCONTEXTOFF);
	printf("#define CONTEXTMASK 0x%x\n", CONTEXTMASK);
	printf("#define FC_MAP 0x%x\n", FC_MAP);
	printf("#define FC_SP 0x%x\n", FC_SP);
	printf("#define LEDOFF 0x%x\n", LEDOFF);
	printf("#define ENABLEOFF 0x%x\n", ENABLEOFF);
	printf("#define BUSERROFF 0x%x\n", BUSERROFF);
	printf("#define PMREALBITS 0x%x\n", PMREALBITS);

	printf("\n");
	/* Page map entry for page zero */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL;
	mapper.pgmapent.pm_type		= MPM_MEMORY;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= 0;
	printf("#define PME_MEM_0 0x%x\n", mapper.longval);

	/* Page map entry for page 800 */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL;
	mapper.pgmapent.pm_type		= MPM_MEMORY;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= 0x800/BYTESPERPG;
	printf("#define PME_MEM_800 0x%x\n", mapper.longval);

	/* Page map entry for timer chip */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_IO;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
#ifdef VME
	mapper.pgmapent.pm_page		= VIOPG_TIMER;
#else  VME
	mapper.pgmapent.pm_page		= MIOPG_TIMER;
#endif VME
	printf("#define PME_TIMER 0x%x\n", mapper.longval);

	/* Page map entry for PROM 0 */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL;
	mapper.pgmapent.pm_type		= VPM_IO;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
#ifdef VME
	mapper.pgmapent.pm_page		= VIOPG_PROM;
#else  VME
	mapper.pgmapent.pm_page		= MIOPG_PROM;
#endif VME
	printf("#define PME_PROM 0x%x\n", mapper.longval);

	/* Page map entry for the canonical invalid page */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_NONE;
	mapper.pgmapent.pm_type		= VPM_MEMORY;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= 0;
	printf("#define PME_INVALID 0x%x\n", mapper.longval);

	/* Page map entry for parallel port */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= MPM_IO;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= MIOPG_PARALLEL;
	printf("#define PME_PARALLEL 0x%x\n", mapper.longval);

	/* Page map entry for Data Ciphering Processor chip */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_IO;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
#ifdef VME
	mapper.pgmapent.pm_page		= VIOPG_DES;
#else  VME
	mapper.pgmapent.pm_page		= MIOPG_DES;
#endif VME
	printf("#define PME_DES 0x%x\n", mapper.longval);

	/*
	 * Page map entries for color board registers
	 */
#define SCBase	VME_COLOR_PHYS

	/* Page map entry for color status register */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_VME_COLOR;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= ((int)&cg2->status) >> BYTES_PG_SHIFT;
	printf("#define PME_COLOR_STAT 0x%x\n", mapper.longval);

	/* Page map entry for color zoom register */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_VME_COLOR;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= ((int)&cg2->zoom) >> BYTES_PG_SHIFT;
	printf("#define PME_COLOR_ZOOM 0x%x\n", mapper.longval);

	/* Page map entry for color word pan register */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_VME_COLOR;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= ((int)&cg2->wordpan) >>
						BYTES_PG_SHIFT;
	printf("#define PME_COLOR_WPAN 0x%x\n", mapper.longval);

	/* Page map entry for color pixel pan register */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_VME_COLOR;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= ((int)&cg2->pixpan) >> BYTES_PG_SHIFT;
	printf("#define PME_COLOR_PPAN 0x%x\n", mapper.longval);

	/* Page map entry for color variable zoom register */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_VME_COLOR;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= ((int)&cg2->varzoom) >>
						BYTES_PG_SHIFT;
	printf("#define PME_COLOR_VZOOM 0x%x\n", mapper.longval);

	/* Page map entry for color maps */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_VME_COLOR;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= ((int)&cg2->redmap[0]) >>
						BYTES_PG_SHIFT;
	printf("#define PME_COLOR_MAPS 0x%x\n", mapper.longval);

	/* Page map entry for color mask register */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 1;
	mapper.pgmapent.pm_permissions	= PMP_ALL
					  - PMP_SUP_EXECUTE - PMP_USER_EXECUTE;
	mapper.pgmapent.pm_type		= VPM_VME_COLOR;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= ((int)&cg2->ppmask) >> BYTES_PG_SHIFT;
	printf("#define PME_COLOR_MASK 0x%x\n", mapper.longval);

	/*
	 * Fields from sunmon.h
	 */
	printf("\n");
	printf("#define INITSP 0x%x\n", INITSP);
	printf("#define USERCODE 0x%x\n", USERCODE);
	printf("#define PROMSIZE 0x%x\n", PROMSIZE);
	printf("#define INUARTA 0x%x\n", INUARTA);
	printf("#define INUARTB 0x%x\n", INUARTB);
	printf("#define NMIFREQ 0x%x\n", NMIFREQ);

	/* 
	 * Fields from globram.h
	 */
	printf("\n");
	printf("#define g_bestack 0x%x\n", &gp->g_bestack);
	printf("#define sizeofbestack 0x%x\n", sizeof(gp->g_bestack));
	printf("#define g_bestack_vor 0x%x\n", &gp->g_bestack.VOR);
	printf("#define g_beregs 0x%x\n", &gp->g_beregs[0]);
	printf("#define g_because 0x%x\n", &gp->g_because);
	printf("#define g_memorysize 0x%x\n", &gp->g_memorysize);
	printf("#define g_resetaddr 0x%x\n", &gp->g_resetaddr);
	printf("#define g_resetaddrcomp 0x%x\n", &gp->g_resetaddrcomp);
	printf("#define g_resetmap 0x%x\n", &gp->g_resetmap);
	printf("#define g_resetmapcomp 0x%x\n", &gp->g_resetmapcomp);
	printf("#define g_prevkey 0x%x\n", &gp->g_prevkey);
	printf("#define g_debounce 0x%x\n", &gp->g_debounce);
	printf("#define g_nmiclock 0x%x\n", &gp->g_nmiclock);
	printf("#define g_echo 0x%x\n", &gp->g_echo);
	printf("#define g_insource 0x%x\n", &gp->g_insource);
	printf("#define g_outsink 0x%x\n", &gp->g_outsink);
	printf("#define g_translation 0x%x\n", &gp->g_translation);
	printf("#define g_keybid 0x%x\n", &gp->g_keybid);
	printf("#define g_keybuf 0x%x\n", &gp->g_keybuf[0]);
	printf("#define g_font 0x%x\n", &gp->g_font);
	printf("#define g_linebuf 0x%x\n", &gp->g_linebuf[0]);
	printf("#define g_lineptr 0x%x\n", &gp->g_lineptr);
	printf("#define g_linesize 0x%x\n", &gp->g_linesize);
	printf("#define gp 0x%x\n", gp);
	printf("#define g_keyrinit 0x%x\n", &gp->g_keyrinit);
	printf("#define g_keyrtick 0x%x\n", &gp->g_keyrtick);
	printf("#define g_busbuf 0x%x\n", &gp->g_busbuf);
	printf("#define g_diag_state 0x%x\n", &gp->g_diag_state);
	printf("#define g_enable 0x%x\n", &gp->g_enable);
	printf("#define g_leds 0x%x\n", &gp->g_leds);
	printf("#define g_keybzscc 0x%x\n", &gp->g_keybzscc);
	printf("#define g_inzscc 0x%x\n", &gp->g_inzscc);
	printf("#define g_fbthere 0x%x\n", &gp->g_fbthere);
	printf("#define g_fbtype 0x%x\n", &gp->g_fbtype);

	/*
	 * Fields from dpy.h
	 */
	printf("\n");
	printf("#define GXBase 0x%x\n", &GXBase);
	printf("#define fbaddr 0x%x\n", &fbaddr);

	/*
	 * Fields from am9513.h
	 */
	printf("\n");
	printf("#define CLK_RESET 0x%x\n", CLK_RESET);
	printf("#define CLK_LOAD_ARM 0x%x\n", CLK_LOAD_ARM);
	printf("#define CLK_LOAD 0x%x\n", CLK_LOAD);
	printf("#define CLK_DISARM_SAVE 0x%x\n", CLK_DISARM_SAVE);
	printf("#define CLK_16BIT 0x%x\n", CLK_16BIT);
	printf("#define CLK_CLEAR 0x%x\n", CLK_CLEAR);
	printf("#define CLK_ACC_MODE 0x%x\n", CLK_ACC_MODE);
	printf("#define CLK_LAST 0x%x\n", CLK_LAST);
	printf("#define CLK_ALL 0x%x\n", CLK_ALL);
	printf("#define CLKM_DEFAULT 0x%x\n", CLKM_DEFAULT);
	printf("#define CLKM_DIV_BY_1 0x%x\n", CLKM_DIV_BY_1);
	printf("#define CLKM_TOGGLE 0x%x\n", CLKM_TOGGLE);
	printf("#define	CLKS_BIT_TIMER_NMI 0x%x\n", CLKS_BIT(TIMER_NMI));
	printf("#define clk_cmd 0x%x\n", &clkp->clk_cmd);
	printf("#define clk_data 0x%x\n", &clkp->clk_data);

	/*
	 * Fields from suntimer.h
	 */
	printf("\n");
	printf("#define TIMER_NMI 0x%x\n", TIMER_NMI);

	/*
	 * Fields from enable.h
	 */
	printf("\n");
	
	/* Enable register mask -- notboot */
	enabler.shortval = 0;
	enabler.enablereg.ena_notboot	= 1;
	printf("#define ena_notboot 0x%x\n", enabler.shortval);

	/* Enable register mask -- parity checking */
	enabler.shortval = 0;
	enabler.enablereg.ena_par_check	= 1;
	printf("#define ena_par_check 0x%x\n", enabler.shortval);
	
	/* Enable register mask -- parity generation */
	enabler.shortval = 0;
	enabler.enablereg.ena_par_gen	= 1;
	printf("#define ena_par_gen 0x%x\n", enabler.shortval);
	
	/* Enable register mask -- interrupts */
	enabler.shortval = 0;
	enabler.enablereg.ena_ints	= 1;
	printf("#define ena_ints 0x%x\n", enabler.shortval);

	/*
	 * Fields from m68vectors.h
	 */
	printf("\n");
	printf("#define EVEC_RESET 0x%x\n", EVEC_RESET);
	printf("#define EVEC_DOG 0x%x\n", EVEC_DOG);
	printf("#define EVEC_BOOTING 0x%x\n", EVEC_BOOTING);
	printf("#define EVEC_BUSERR 0x%x\n", EVEC_BUSERR);
	printf("#define	EVEC_LEVEL7 0x%x\n", EVEC_LEVEL7);
	printf("#define EVEC_TRAP1 0x%x\n", EVEC_TRAP1);
	printf("#define EVEC_TRAPE 0x%x\n", EVEC_TRAPE);
	printf("#define EVEC_KCMD 0x%x\n", EVEC_KCMD);
	printf("#define EVEC_ABORT 0x%x\n", EVEC_ABORT);
	printf("#define VOR_OFFSET 0x%x\n", VOR_OFFSET);
	printf("#define sizeofintstack 0x%x\n", sizeof(struct intstack));
	printf("#define i_vor 0x%x\n", &ip->i_vor);

	/*
	 * Fields from zsreg.h
	 */
	printf("\n");
	printf("#define ZSRR0_BREAK 0x%x\n", ZSRR0_BREAK);
	printf("#define ZSRR0_RX_READY 0x%x\n", ZSRR0_RX_READY);
	printf("#define ZSRR0_TX_READY 0x%x\n", ZSRR0_TX_READY);
	printf("#define ZSWR0_RESET_STATUS 0x%x\n", ZSWR0_RESET_STATUS);
	printf("#define zscc_control 0x%x\n", &zp->zscc_control);
	printf("#define zscc_data 0x%x\n", &zp->zscc_data);
	printf("#define sizeofzscc 0x%x\n", sizeof(struct zscc_device));
	printf("#define A_CONTROL 0x%x\n", &(zp+1)->zscc_control);
	printf("#define A_DATA 0x%x\n", &(zp+1)->zscc_data);
	printf("#define B_CONTROL 0x%x\n", &(zp+0)->zscc_control);

	/*
	 * Fields from sunromvec.h
	 */
	printf("\n");
	printf("#define v_printf 0x%x\n", &RomVecPtr->v_printf);

	/*
	 * Fields from diag.h
	 */
	printf("\n");
	printf("#define PROMBIT 0x%x\n", PROMBIT);
	printf("#define CXBIT 0x%x\n", CXBIT);
	printf("#define SEGBIT 0x%x\n", SEGBIT);
	printf("#define PMBIT 0x%x\n", PMBIT);
	printf("#define MEMBIT 0x%x\n", MEMBIT);
	printf("#define DESFOUND 0x%x\n", DESFOUND);
	printf("#define DESBIT 0x%x\n", DESBIT);
	printf("#define MCONBIT 0x%x\n", MCONBIT);
	printf("#define MADDRBIT 0x%x\n", MADDRBIT);
	printf("#define MTRANBIT 0x%x\n", MTRANBIT);
	printf("#define LEDQUICK 0x%x\n", LEDQUICK);
	printf("#define LEDLONG 0x%x\n", LEDLONG);

	/*
	 * Fields from am8068.h
	 */
	printf("\n");
	printf("#define DESSELOFF 0x%x\n", &dp->d_selector);
	printf("#define DESREGOFF 0x%x\n", &dp->d_reg);
	printf("#define DESR_IO 0x%x\n", DESR_IO);
	printf("#define DESR_CMD_STAT 0x%x\n", DESR_CMD_STAT);
	printf("#define DESR_MODE 0x%x\n", DESR_MODE);
	printf("#define DESC_RESET 0x%x\n", DESC_RESET);
	printf("#define DESC_STOP 0x%x\n", DESC_STOP);
	printf("#define DESC_START 0x%x\n", DESC_START);
	printf("#define DESC_START_DEC 0x%x\n", DESC_START_DEC);
	printf("#define DESC_START_ENC 0x%x\n", DESC_START_ENC);
	printf("#define DESC_LOAD_E_KEY 0x%x\n", DESC_LOAD_E_KEY);
	printf("#define DESC_LOAD_D_KEY 0x%x\n", DESC_LOAD_D_KEY);
	printf("#define DESS_MST_FLAG 0x%x\n", DESS_MST_FLAG);
	printf("#define DESS_SLAVE_FLAG 0x%x\n", DESS_SLAVE_FLAG);
	printf("#define DESS_AUX_FLAG 0x%x\n", DESS_AUX_FLAG);
	printf("#define DESS_PAR 0x%x\n", DESS_PAR);
	printf("#define DESS_LPAR 0x%x\n", DESS_LPAR);
	printf("#define DESS_BUSY 0x%x\n", DESS_BUSY);
	printf("#define DESS_CMD_PEND 0x%x\n", DESS_CMD_PEND);
	printf("#define DESS_STARTED 0x%x\n", DESS_STARTED);
	printf("#define DESM_ECB 0x%x\n", DESM_ECB);
	printf("#define DESM_CFB 0x%x\n", DESM_CFB);
	printf("#define DESM_CBC 0x%x\n", DESM_CBC);
	printf("#define DESM_ME_SC 0x%x\n", DESM_ME_SC);
	printf("#define DESM_MC_SE 0x%x\n", DESM_MC_SE);
	printf("#define DESM_M_ONLY 0x%x\n", DESM_M_ONLY);
	printf("#define DESM_DECRYPT 0x%x\n", DESM_DECRYPT);
	printf("#define DESM_ENCRYPT 0x%x\n", DESM_ENCRYPT);

	/*
 	 * Fields from montrap.h
	 */
	printf("\n");
	printf("#define mis_d0 0x%x\n", &mp->mis_d0);
	printf("#define mis_a7 0x%x\n", &mp->mis_a7);
	printf("#define mis_usp 0x%x\n", &mp->mis_usp);
	printf("#define mis_sfc 0x%x\n", &mp->mis_sfc);
	printf("#define mis_dfc 0x%x\n", &mp->mis_dfc);
	printf("#define mis_vbase 0x%x\n", &mp->mis_vbase);
	printf("#define mis_scon 0x%x\n", &mp->mis_scon);
	printf("#define mis_ucon 0x%x\n", &mp->mis_ucon);
	printf("#define mis_sr 0x%x\n", &mp->mis_sr);
	printf("#define mis_vor 0x%x\n", &mp->mis_vor);
	printf("#define mis_size 0x%x\n", sizeof (struct monintstack));

	exit(0);
}

