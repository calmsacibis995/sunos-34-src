
#ifndef lint
/*static  char sccsid[] = "@(#)diagmenus.c 1.1 86/09/27 Copyr 1985 Sun Micro";*/
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


/*
 * diagmenus.c:  Menu test routines called from commands.c in 'X' command.
 */

#include "../h/systypes.h"
#include "../sun3/sunmon.h"
#include "../h/globram.h"
#include "../sun3/cpu.misc.h"
#include "../sun3/cpu.addrs.h"
#include "../dev/zsreg.h"
#include "../sun3/m68vectors.h"
#include "../h/sunromvec.h"
#include "../dev/saio.h"
#include "../dev/promether.h"
#include "../sun3/machdep.h"
#include "../dev/amd_ether.h"
#include "../diag/diagmenus.h"

extern	struct sptab speedtab[];
extern	struct sptab {int speeds, counts};


/* Subroutine to perform bootpath test for all storage class peripheral
 * devices, i.e., those which use call boot(cmd).  The routine copies
 * the command string whose pointer is passed then calls the boot(cmd)
 * routine, checks its return argument for a boot read error or lack
 * of it, and reports "bootpath test failed" or "bootpath test passed".
 * Its scope of testing then is no better than the error checking and
 * reporting of each device driver.
 */


boot_test(string)
	char *string;
{
	int  boot_try, error, errors = 0, pass = 0;
	char *st_ptr;

	/* first, copy the boot command string passed to the command
	   buffer */

	get_options();				/* Get test options */
	if (gp->g_option == 'Q')	
		return('Q');			/* quit test */
	do {
		error = 0;
		st_ptr = string;
          	gp->g_lineptr = gp->g_linebuf;
          	while (*(gp->g_lineptr)++ = *st_ptr++) ;
          	gp->g_lineptr = gp->g_linebuf;
          	boot_try = boot(gp->g_lineptr); 	/* call boot routine */

          	if (boot_try <= 0){ 
			error = 1;
			++errors;
		}
	} while (endtest(error, ++pass, errors));
}
#ifndef	M25
/* 
 * Intel Ethernet Tests
 */

ether_test()

{
	char 	cmd;
	register struct ereg       *eregp = (struct ereg *)ETHER_BASE;
	register struct etherblock *blockp = (struct etherblock *)ETHERMEM;
	int	input, fifo_lim;
	int	blocksize = ETHERBUFSIZE;
	int	shortblocksize = 18; /* max size for encoder and external */
				     /* loopback tests as required by the */
				     /* Intel chip. Is in the manual under*/
				     /* loopback (2.11.5 pg 2-57 of the   */
				     /* Intel LAN components users manual */
				     /* Intel order no. 230814-001)       */

	for (;;) {
		test_hdr("Intel Ethernet", 0);
		display_opt("l", "Local Loopback");
		display_opt("e", "Encoder Loopback");
		display_opt("x", "External Loopback");
		if ((cmd = get_cmd()) == 'Q') 	/* get command char */
			return (cmd);
		if (cmd != 'L' && cmd != 'E' && cmd != 'X') {
			invalid_selmsg();
			continue;			/* Invalid commnad */
		}
		get_options();                          /* Get test options */ 
        	if (gp->g_option == 'Q')        
                	return('Q');                    /* quit test */
/*
 * Map in memory buffer space for testing Intel Ethernet chip and execute
 * test routine.
 */
 		/* First, setup segment map addresses for 32 pages, 2 entries */
 
		fifo_lim = 8;
		setsmreg(0x0f080000, 0xc0);	/* 16 pages */
		setsmreg(0x0f0a0000, 0xc1);	/* 16 pages */
		map(0xf080000, 32*PAGESIZE, ETHERMEM, PM_MEM); /*map 32 pages*/

		if (cmd == 'L')			/* local loopback */
			ether_loop(eregp, blockp, blocksize, fifo_lim, 1, 1, 0);
		else if (cmd == 'E')		/* encoder loopback */
			ether_loop(eregp, blockp, shortblocksize, fifo_lim,
				   0, 1, 0);	
		else 			           /* external loopback */
			ether_loop(eregp, blockp, shortblocksize, fifo_lim, 
				   0, 1, 1);
	}
}						/* end function */

/*
 *	Generic Ethernet loopback test
 *
 *	Local    => intloop = 1, extloop = 1, cable = 0
 *	Encoder  => intloop = 0, extloop = 1, cable = 0
 *	External => intloop = 0, extloop = 1, cable = 1
 */

ether_loop(eregp, blockp, blocksize, fifo_lim, intloop, extloop, cable)
	int	fifo_lim, blocksize, intloop, extloop, cable;
	register struct ereg            *eregp;
	register struct etherblock      *blockp;
{
        int	passed_ok, error, pass = 0, errors = 0;

        do {
		error = 0;
        	passed_ok = etherconf(eregp, blockp, intloop, extloop, cable,
			              fifo_lim, 10);
		if (passed_ok)
			passed_ok = loopback(eregp, blockp, blocksize);

		if (passed_ok == 0) {
			error = 1;
			++errors;
		}
	} while (endtest(error, ++pass, errors));
}

etheraddr	myaddr = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};

etherca(blockp, eregp)
	struct etherblock	*blockp;
	register struct ereg	*eregp;
{
	eregp->obie_ca = 1;	/* toggle CA for us */
	eregp->obie_ca = 0;
}


/*
 * Reset the EDLC chip and get it ready for commands.
 * Returns 1 for OK, 0 for nogood.
 */

int
ether_reset(eregp, blockp)
	register struct ereg		*eregp;
	register struct etherblock	*blockp;
{
	struct escp	*scpp = &blockp->scp;	 
	struct eiscp	*iscpp = &blockp->iscp;
	struct escb	*scbp = &blockp->scb;
	long		i;
	/*
	 * Take the chip off the cable, and set up maps and parity.
	 */
	BCLEAR(eregp);
	BCLEAR(scpp);		/* clear it first */
	scpp->iscp_addr = to_mieaddr(blockp, (caddr_t)iscpp);
	BCLEAR(iscpp);		/* clear it first */
	iscpp->iscp_busy = 1;		
	iscpp->iscb_base = to_mieaddr(blockp, blockp);
	iscpp->iscb_offset = to_mieoff(blockp, scbp);
	BCLEAR(scbp);		/* clear it first */
	eregp->obie_noreset = 0;
	DELAY(1);
	eregp->obie_noreset = 1;
	DELAY(1);


       /* 
 	* Initialize SCP (0xFFFFF6 in EDLC address space) 
	*/

	(*(struct escp *)SCP_ADDR) = blockp->scp; /* initialize SCP address */
						/* at 0FFFFFFC */
	
	etherca(blockp, eregp);			/* toggle CA to start ball */
	i = 100000;
	while(iscpp->iscp_busy && --i);

	if (i == 0){
		return(0);
	}

	return(1);
};


/*
 * Reset and configure the Ethernet chip.
 * Return 0 for error, 1 for OK.
 */
etherconf(eregp, blockp, intloop, extloop, cable, fifolim, framelen)
	register struct ereg		*eregp;
	register struct etherblock	*blockp;
	int		intloop, extloop, cable, fifolim, framelen;
{
	register struct conf_ext	*confp = &blockp->cbl[0].cb_ext.conf;
	struct escb	*scbp = &blockp->scb;

	if (!ether_reset(eregp, blockp)) {
		display_msg("Reset bad\n"); 
		return(0);	
	}

	/* Now put us on the cable if desired. */
	eregp->obie_noloop = cable;

	BCLEAR(confp);

	confp->conf_fifolim	= fifolim;
	confp->conf_count	= 12;
	confp->conf_extloop	= extloop;
	confp->conf_intloop	= intloop;
	confp->conf_preamlen	= 2;
	confp->conf_addrlen	= 6;
	confp->conf_framespace	= 96;
	confp->conf_numretries	= 15;
	confp->conf_slottime_hi	= 512 >> 8;
	confp->conf_slottime_lo	= (u_char) 512 & 0xFF;
	confp->conf_minframelen	= framelen;

	if(!ethercommand(eregp, blockp, CMD_CONFIGURE)){
		display_msg("config: config bad\n"); 
		return(0);
	} else {
		if (scbp->scb_cx)	/* tell chip type */
			display_msg("config: B0 chip\n"); 

		bcopy(myaddr, blockp->cbl[0].cb_ext.iaddr, sizeof(etheraddr));
		if(!ethercommand(eregp, blockp, CMD_IASETUP)){
			display_msg("ia setup bad\n"); 
			return(0);
		} else {
			return(1);
		}
	}
}

/*
 * Execute one command
 */
ethercommand(eregp, blockp, cmd)
	register struct ereg		*eregp;
	register struct etherblock	*blockp;
	register			cmd;
{
	register struct escb	*scbp = &blockp->scb;
	register struct ecb	*cbp = &blockp->cbl[0].cb;
	register long		timer;


	*(u_long *)cbp = 0;			/* clear block header */
	cbp->cb_el = 1;				/* is last block */
	cbp->cb_link = 0;			/* no address */
	cbp->cb_cmd = cmd;			/* use command given */

	timer = 500000;
	while(--timer && scbp->scb_cuc);	/* wait for CU */
	if (!timer){
		display_msg("\ncommand: CU w timeout\n");
		return(0);
	}
		
	scbp->scb_cbloff = to_mieoff(blockp, cbp);	/* set up offset */
	scbp->scb_cuc = CUC_START;		/* start CU on cbl */

	etherca(blockp, eregp);				/* toggle CA */

	timer = 500000;
	while(--timer && !cbp->cb_c);		/* wait for cmd to finish */
	if (!timer){
		display_msg("\ncommand: c timeout\n"); 
		return(0);
	}

	timer = 500000;
	while(--timer && scbp->scb_cuc);	/* wait for CU */
	if (!timer){
		display_msg("\ncommand: CU f timeout\n"); 
		return(0);
	}

	scbp->scb_cuc = CUC_NOP;		/* not necessary, why not*/
	scbp->scb_cxack = scbp->scb_cx;		/* do acknowledge */
	scbp->scb_frack = scbp->scb_fr;		/* do acknowledge */
	scbp->scb_cnrack = scbp->scb_cnr;	/* do acknowledge */
	scbp->scb_rnrack = scbp->scb_rnr;	/* do acknowledge */
	etherca(blockp, eregp);				/* toggle CA */

	timer = 500000;
	while(--timer && scbp->scb_cuc);	/* wait for CU */
	if (!timer){
		display_msg("\ncommand: CU ack timeout\n"); 
		return(0);
	} else {
		return(1);
	}
}


/*
 * Convert a CPU virtual address into an Ethernet virtual address.
 *
 * For Multibus, we assume it's in the Ethernet's memory space, and we
 * just subtract off the start of the memory space (==es).  For Model 50,
 * the Ethernet chip has full access to supervisor virtual memory.
 */
mieaddr_t
to_mieaddr(blockp, cp)
	struct etherblock *blockp;
	caddr_t cp;
{
	union {
		int	n;
		char	c[4];
	} a, b;

	a.n = (int)cp;
	b.c[0] = a.c[3];
	b.c[1] = a.c[2];
	b.c[2] = a.c[1];
	b.c[3] = 0;
	return (b.n);
}

/*
 * Convert an EDLC virtual address to a CPU virtual address
 */
caddr_t
from_mieaddr(blockp, n)
	struct etherblock *blockp;
	mieaddr_t n;
{
	union {
		long	n;
		char	c[4];
	} a, b;

	a.n = n;
	b.c[0] = 0;
	b.c[1] = a.c[2];
	b.c[2] = a.c[1];
	b.c[3] = a.c[0];
	return ((caddr_t)b.n);
}

/*
 * Convert a CPU virtual address into a 16-bit offset for the Ethernet
 * chip.
 *
 * This is the same for Onboard and Multibus, since the offset is based
 * on the absolute address supplied in the initial system configuration
 * block -- which we customize for Multibus or Onboard.
 */
mieoff_t
to_mieoff(blockp, addr)
	register struct etherblock *blockp;
	caddr_t addr;
{
	union {
		short	s;
		char	c[2];
	} a, b;

	a.s = (short)(addr - (caddr_t)blockp);
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (b.s);
}

unsigned int MAXBUF = NTXBUF;		/* Hack to simplify debug */

loopback(eregp, blockp, blocksize)
	register struct ereg		*eregp;
	register struct etherblock	*blockp;
	int				blocksize;
{
	struct escb		*scbp = &blockp->scb;
	struct cbl		*cblp = blockp->cbl;
	struct etbd		*tbdp = blockp->tbd;
	struct erfd		*rfdp = blockp->rfd;
	struct erbd		*rbdp = blockp->rbd;
	struct etxcb		*txp;
	register long		i;
	register int		index;
	int count;

		/* clear all lists */

	bzero(scbp, sizeof(blockp->scb));
	bzero(cblp, sizeof(blockp->cbl));
	bzero(tbdp, sizeof(blockp->tbd));
	bzero(rfdp, sizeof(blockp->rfd));
	bzero(rbdp, sizeof(blockp->rbd));

		/* set up lists */

	for (i = 0; i < MAXBUF ; i++) {

		/* command block for transmit */

		cblp[i].cb.cb_cmd = CMD_TRANSMIT;
		cblp[i].cb.cb_link = to_mieoff(blockp, &cblp[i+1]);

		/* command tx extension block for transmit */

		cblp[i].cb_ext.tx.tx_bdptr = 
			blocksize ? to_mieoff(blockp, &tbdp[i]) : 0xffff;
		bcopy(myaddr, cblp[i].cb_ext.tx.tx_addr, sizeof(etheraddr));
		cblp[i].cb_ext.tx.tx_type = 1<<i;

		/* transmit buffer descriptor for tx */

		if (blocksize) {
			tbdp[i].tbd_eof = 1;
			tbdp[i].tbd_count_hi = blocksize >> 8;
			tbdp[i].tbd_count_lo = blocksize;
			tbdp[i].tbd_addr = to_mieaddr(blockp, blockp->txbuf[i]);
		}

		/* received frame descriptor for the receive */

		rfdp[i].rfd_link = to_mieoff(blockp, &rfdp[i+1]);
		rfdp[i].rfd_bdptr = 0xffff;

		/* received buffer descriptor for rx */

		rbdp[i].rbd_bdptr = to_mieoff(blockp, &rbdp[i+1]);
		rbdp[i].rbd_addr = to_mieaddr(blockp, blockp->rxbuf[i]);
		rbdp[i].rbd_size_hi = ETHERBUFSIZE >> 8; 
		rbdp[i].rbd_size_lo = ETHERBUFSIZE & 0xFF;
	}

		/* now do special setups in lists */

	cblp[MAXBUF -1].cb.cb_el = 1;		/* terminate command list */
	rfdp[0].rfd_bdptr = to_mieoff(blockp, rbdp);
	rfdp[MAXBUF -1].rfd_el = 1;		/* terminate rfd list */

		/* setup two bogus rbd's for step B0 chips to eat */

	rbdp[MAXBUF].rbd_bdptr = to_mieoff(blockp, &rbdp[MAXBUF + 1]);
	rbdp[MAXBUF].rbd_addr = to_mieaddr(blockp, blockp->rxbogus[0]);
	rbdp[MAXBUF].rbd_size_hi = 0;
	rbdp[MAXBUF].rbd_size_lo = 16;

	rbdp[MAXBUF + 1].rbd_addr = to_mieaddr(blockp, blockp->rxbogus[1]);
	rbdp[MAXBUF + 1].rbd_size_hi = 0;
	rbdp[MAXBUF + 1].rbd_size_lo = 16;
	rbdp[MAXBUF + 1].rbd_el = 1;

		/* try to transmit this stuff and receive it */

	i = 500000;				/* wait for CU */
	while(--i && scbp->scb_cuc);

	if (i == 0) {
		display_msg("loop:CU bcw timeout\n"); 
		return(0);
	}

	i = 500000;				/* wait for CU and RU idle */
	while(--i && (scbp->scb_cus != CUS_IDLE || scbp->scb_rus != RUS_IDLE));

	if (i == 0) {
		display_msg("loop:CU/RU bsw timeout\n");
		return(0);
	}


	scbp->scb_cuc = CUC_START;
	scbp->scb_ruc = RUC_START;
	scbp->scb_cbloff = to_mieoff(blockp, cblp);
	scbp->scb_rfaoff = to_mieoff(blockp, rfdp);

	etherca(blockp, eregp);		/* knock their lights out */

	for( i = 1000; i ; --i){
		lfill(blockp->dummyfill,
			LONGSIZE(blockp->dummyfill), 0x12345678);
		if (index = lcheck(blockp->dummyfill,
			LONGSIZE(blockp->dummyfill), 0x12345678)){
			if (gp->g_option != 'N')
				printf("loop%d: bother at 0x%x\n",i,
					LONGSIZE(blockp->dummyfill)>>2 - index);
			return(0);
		}
		if (scbp->scb_cus == CUS_IDLE &&
			scbp->scb_rus == RUS_NORESOURCES){
			break;
		}
	}
	if (i == 0) {
		display_msg("loop:CU/RU ew timeout\n"); 
		return(0);
	} 

	/* acknowledge the command */

	scbp->scb_cuc = CUC_NOP;		/* not necessary, why not*/
	scbp->scb_cxack = scbp->scb_cx;		/* do acknowledge */
	scbp->scb_frack = scbp->scb_fr;		/* do acknowledge */
	scbp->scb_cnrack = scbp->scb_cnr;	/* do acknowledge */
	scbp->scb_rnrack = scbp->scb_rnr;	/* do acknowledge */
	etherca(blockp, eregp);				/* toggle CA */

	/* now check lists/buffers for okness */
	if(scbp->scb_cuc != CUC_NOP)
		display_msg("loop: scb CU not NOP\n"); 

	if (scbp->scb_ruc != RUC_NOP)
		display_msg("loop: scb RU not NOP\n"); 

	if (scbp->scb_cus != CUS_IDLE)
		display_msg("loop: scb CU not IDLE\n"); 

	if (scbp->scb_rus != RUS_NORESOURCES)
		display_msg("loop: scb RU not NO RES\n"); 

	if (scbp->scb_cbloff != to_mieoff(blockp, cblp) && gp->g_option != 'N')
		printf("loop: scb @cbl e(0x%x) o(0x%x)\n",
			to_mieoff(blockp, cblp), scbp->scb_cbloff); 

	if (scbp->scb_rfaoff != to_mieoff(blockp, rfdp) && gp->g_option != 'N')
		printf("loop: scb @rfa e(0x%x) o(0x%x)\n",
			to_mieoff(blockp, rfdp), scbp->scb_rfaoff);

	if (scbp->scb_crcerrs || scbp->scb_alnerrs || scbp->scb_rscerrs ||
	    scbp->scb_ovrnerrs)
		display_msg("loop: scb stats\n"); 

	for (i = 0; i < MAXBUF ; cblp++, tbdp++, rfdp++, rbdp++, i++){
		if (blocksize) {
			if (index = lcmp(blockp->txbuf[i],
				blockp->rxbuf[i], blocksize)){
				index = blocksize/sizeof(long) - index;
				if (gp->g_option != 'N')
				printf("loop[%x]: data +0x%x e(0x%x) o(0x%x)\n",
				   index,
				   *(((u_long *)(blockp->txbuf[i]))+index),
				   *(((u_long *)(blockp->rxbuf[i]))+index)); 
				return(0);
			}
		}
	}
	return(1);
}
#endif	M25
#ifdef	M25
/*
 * AMD Ethernet Tests
 */

amd_ether_test()

{
	char 	cmd;
	int	input, wdata;
	u_long	vadrs;

	for (;;) {
		test_hdr("AMD Ethernet", 0);
		display_opt("w", "Wr/Rd CSR1 Reg");
                display_opt("l", "Local Loopback");
                display_opt("x", "External Loopback");
		if ((cmd = get_cmd()) == 'Q')   /* get command char */
			return (cmd);
		if (cmd != 'L' && cmd != 'W' && cmd != 'X') {
			invalid_selmsg();
			continue;			/* Invalid command */
		}
		if (cmd == 'W') {
                	skipblanks();   /* skip spaces */ 
			wdata = getnum();	/* get write pattern */
		}
		get_options();                          /* Get test options */ 
        	if (gp->g_option == 'Q')        
                	return('Q');                    /* quit test */
/*
 * Map in memory buffer space for testing AMD Ethernet chip and execute
 * test routine.
 */
 		/* First, setup segment map addresses for 16 pages, 1 entry */
 
		setsmreg(0x0f080000, 0xc0);	/* 16 pages */
		map(0xf080000, 16*PAGESIZE, ETHERMEM, PM_MEM); /*map 16 pages*/

		vadrs = (u_long)(ETHERMEM + 0xF000000);
		if (cmd == 'L')			/* local loopback */
			amd_loop_test(1, vadrs);
		else if (cmd == 'X')           /* external loopback */
			amd_loop_test(2, vadrs); 
		else if (cmd == 'W')
			amd_wr_rd_test(wdata);
		else
			return('Q');
	  }
}						/* end function */

/*
 * Generic AMD Ethernet Loopback Test
 */

amd_loop_test(mode,vadrs)

	u_short	mode;
	u_long	vadrs;

{
	int	error, pass = 0, errors =0;

	do {
		error = 0;
		if (amd_ether_loop(mode,vadrs) != 0) {
			error = 1;
			++errors;
		}

		if (mode == 1)  
			display_msg("Local");
		else 
			display_msg("External");

	} while (endtest(error, ++pass, errors));
}

/*
 * amd_wr_rd_test
 * 
 * Tests ability to address and write/read access control/status register
 * 0 (CSR0) and 1 (CSR1) within the Am7990 Ethernet chip.
 */

amd_wr_rd_test(wdata)
	
	u_short	wdata;
{
	u_short	*reg_addrs, *reg_data, rdata;
	int	pass, error, errors = 0;
	
	pass = 0;
	reg_addrs = (u_short *)AMD_ETHER_BASE + AMD_E_RAP;
	reg_data = (u_short *)AMD_ETHER_BASE + AMD_E_RDP;
	
	do {
	/* Stop AMD ether chip in order to test its data lines */

		error = 0;
		*reg_addrs = AMD_E_CSR0;	/* select CSR0 */
		*reg_data = AMD_E_STOP;	/* output stop control bit to chip */
		DELAY(10);		/* delay for data bus to decay */
		rdata = *reg_data;	/* read back CSR0 data */
		if ((rdata != AMD_E_STOP) && (gp->g_option != 'N')) {
			error = 1;
			++errors;
			printf("CSR0 Wr/Rd Error: exp %x, obs %x\n",AMD_E_STOP,rdata);
		}

		/* Write/Read CSR1 within AMD ether chip */

		*reg_addrs = AMD_E_CSR1;	/* select CSR1 */
		*reg_data = wdata;	/* write CSR1 within AMD ether chip */
		DELAY(10);	/* delay for data bus to discharge */
		rdata = *reg_data;	/* read CSR1 within AMD ether chip */
		if ((rdata != wdata) && (gp->g_option != 'N')) {
			error = 1;
			++errors;
			printf("CSR1 Wr/Rd Error: exp %x, obs %x\n", wdata, rdata);
		}
	} while (endtest(error, ++pass, errors));
}


amd_ether_loop(mode, vadrs)
	
	u_short	mode;
	u_long	vadrs;  
{
	u_short	*radrs, *rdata, found, exp, randomseed, loops;
	int all, timeleft, x;
	char error, timeout; 
	char *amd_rx_data;
	char *amd_tx_data;
	struct amd_init *amd_init_block;
	struct amd_rcv_ring *amd_rx_block;
	struct amd_txmt_ring *amd_tx_block;


	radrs = (u_short *)AMD_ETHER_BASE+AMD_E_RAP;
	rdata = (u_short *)AMD_ETHER_BASE+AMD_E_RDP; 
	amd_init_block = (struct amd_init *)vadrs;
	amd_rx_block = (struct amd_rcv_ring *)(vadrs+0x20); 
	amd_tx_block = (struct amd_txmt_ring *)(vadrs+0x030);
	amd_tx_data = (char *)(vadrs+0x100);
	amd_rx_data = (char *)(vadrs+0x200);

	if (mode == 2)			 /* external loopback */
		amd_init_block->mode =  AMD_E_LOOPBACK;
	else					 /* default internal loopback */
		amd_init_block->mode =  AMD_E_INTL | AMD_E_LOOPBACK;
	amd_init_block->padr_lo = 0x200;
	amd_init_block->padr_mid = 0x0;
	amd_init_block->padr_hi = 0x0;
	amd_init_block->ladr_lo = 0x0;
	amd_init_block->ladr_mid_lo = 0x0;
	amd_init_block->ladr_mid_hi = 0x0;
	amd_init_block->ladr_hi = 0x0;
	amd_init_block->rdra_lo = (u_short)((u_long)amd_rx_block & 0xFFFF);
	amd_init_block->rdra_hi = (u_short)(((u_long)amd_rx_block >> 16) & 0xFF);
	amd_init_block->tdra_lo = (u_short)((u_long)amd_tx_block & 0xFFFF);
	amd_init_block->tdra_hi = (u_short)(((u_long)amd_tx_block >> 16) & 0xFF);

	amd_rx_block->rmd0_ladr = (u_short)((u_long)amd_rx_data & 0xFFFF);
	amd_rx_block->rmd1_hadr = (u_short)(((u_long)amd_rx_data >> 16) & 0xFF);
	amd_rx_block->rmd2_bcnt = (u_short)0-40;
				
	/* recomended msg size for loopback*/
	/* The F000 is required by amd in hi bits */

	amd_tx_block->tmd0_ladr = (u_short)((u_long)amd_tx_data & 0xFFFF);
	amd_tx_block->tmd1_hadr = (u_short)(((u_long)amd_tx_data >> 16) & 0xFF);
	amd_tx_block->tmd2_bcnt = (u_short)0-32; 

	/* recomended msg size for loopback*/
	/* The subtraction generates the necessary */
	/* 2's complement number in tmd2_bcnt  */
  
	amd_tx_block->tmd3_tdr = (u_short)0;/* clear any previous error in tmd3 */ 
	error = FALSE;

/*
 * STOP chip in prep to init                     *
 */

	*radrs = AMD_E_CSR0; /* select CSR0 */
	*rdata = AMD_E_STOP;       
	if (*rdata != AMD_E_STOP)
		error = TRUE;

/*
 * LOAD CSR1,2 with the base of the init structure *
 */

	*radrs = AMD_E_CSR1; /* select CSR1 */

	/* contents of pointer to init structure */

	*rdata = (u_short)((u_long)amd_init_block & 0xFFFE);
 
	*radrs = AMD_E_CSR2; /* select CSR2 */

	/* contents of pointer to init structure */
	
	*rdata = (u_short)(((u_long)amd_init_block >> 16) & 0xFF);

	*radrs = AMD_E_CSR3; /* select CSR3 */
	
	/* byte swap, ALE, byte control bits */
      
	*rdata = (u_short) 0;



/*
 * INIT chip					 *
 */

	*radrs = AMD_E_CSR0; /* select CSR0 */
	*rdata = AMD_E_INIT;       
	timeleft = MAXTIME_AMD;

	while ( (timeleft--) && ((found = *rdata) & AMD_E_INITOK) != AMD_E_INITOK)
	{} /* wait for correct action or timeout */
	
	if (!timeleft) {
		error = TRUE; /* failed to init correctly */
		if (gp->g_option != 'N') {
			printf("Initialization failure, CSR0 exp %x, obs %x\n",
				found & AMD_E_INITOK, found);
		}
	}

/*
 * LOAD buffer for TXMIT and CLEAR RX buffer
 */
	loops = 1;
	while ((mayget() == -1) && (loops--) && (!error)) {
		timeout = FALSE;
		for (x = 0; x < 32; ++x) {
			*(amd_tx_data + x) = x;	/* setup write data */
			*(amd_rx_data + x) = 0;	/* clear read data */
		}

		*(amd_tx_data + 0) = 2; /* set destination address */
		*(amd_tx_data + 1) = 0;
		*(amd_tx_data + 2) = 0;
		*(amd_tx_data + 3) = 0;
		*(amd_tx_data + 4) = 0;
		*(amd_tx_data + 5) = 0;

		*(amd_tx_data + 6) = 2; /* set source address */
		*(amd_tx_data + 7) = 0;
		*(amd_tx_data + 8) = 0;
		*(amd_tx_data + 9) = 0;
		*(amd_tx_data + 10) = 0;
		*(amd_tx_data + 11) = 0;
  



/*
 * START chip     
 */

		*radrs = AMD_E_CSR0; /* select CSR0 */
		*rdata = AMD_E_START; /* if we are already started this has no effect */
		if ((*rdata & AMD_E_START) != AMD_E_START)
			error = TRUE;
       
/*
 * Transmit buffer & VERIFY transmittion ok	 *
 */

		amd_tx_block->tmd1_hadr = amd_tx_block->tmd1_hadr | 
			(u_short)AMD_E_STP |    /* Start of Packet mark */
			(u_short)AMD_E_ENP;     /* End of Packet mark   */
				

		timeleft = MAXTIME_AMD_TX;

		/* give receive buffer to lance */

		amd_rx_block->rmd1_hadr = amd_rx_block->rmd1_hadr | (u_short)(AMD_E_OWN);
		/* give Transmit buffer to lance */

		amd_tx_block->tmd1_hadr = amd_tx_block->tmd1_hadr | (u_short)(AMD_E_OWN);

		while ((--timeleft) && (amd_tx_block->tmd1_hadr & (u_short)AMD_E_OWN))
		{}
		
		/* We will want to add tests for other signs that a receive */
		/* was accomplished */

		
		if (!timeleft) {
			error = TRUE; /* failed to confirm tx in time */
			if (gp->g_option != 'N') {
				printf("TX - TIMEOUT  stat=%x rmd1=%x tmd1=%x tmd3=%x\n",*rdata, 
					amd_rx_block->rmd1_hadr,
					amd_tx_block->tmd1_hadr,
					amd_tx_block->tmd3_tdr);
			}
	       	timeout = TRUE;
		}
		else {
			if (amd_tx_block->tmd1_hadr & AMD_E_TXERR) { 
	        		error = TRUE;
	        		if (gp->g_option != 'N') {
						printf("TX - ERROR   stat=%x tmd1=%x tmd3=%x\n",*rdata, 
					amd_tx_block->tmd1_hadr,
					amd_tx_block->tmd3_tdr);
					}
			}
		} 
	      
		if (*rdata & (AMD_E_BABL + AMD_E_MISS + AMD_E_MERR)) { 
			error = TRUE;
			if (gp->g_option != 'N') {
				printf("STATUS - ERROR   stat=%x\n",*rdata);
			} 
		}
	      

    
/*
 * Recieve buffer and verify receive ok          
 */

		timeleft = MAXTIME_AMD_RX;

		while ( (--timeleft) && (amd_rx_block->rmd1_hadr & (u_short)AMD_E_OWN) )
		{}
           /* We will want to add tests for other signs that a receive */
	   /* was accomplished */

		if (!timeleft) {
			error = TRUE; /* failed to receive in time */
			if (gp->g_option != 'N') {
				printf("RX - TIMEOUT  stat=%x rmd1=%x tmd1=%x tmd3=%x\n",*rdata, 
					amd_rx_block->rmd1_hadr,
					amd_tx_block->tmd1_hadr,
					amd_tx_block->tmd3_tdr);
			}
			timeout = TRUE;
		}
		else {
			if (amd_rx_block->rmd1_hadr & AMD_E_RXERR) { /* mask for any errors */
				error = TRUE;
				if (gp->g_option != 'N') {
					printf("RX - ERROR   stat=%x rmd1=%x tmd1=%x tmd3=%x\n",*rdata, 
						amd_rx_block->rmd1_hadr,
						amd_tx_block->tmd1_hadr,
						amd_tx_block->tmd3_tdr);
				}
			}
		} /* end else */	      

		if (*rdata & (AMD_E_BABL + AMD_E_MISS + AMD_E_MERR)) {
			error = TRUE;
			if (gp->g_option != 'N') {
				printf("STATUS - ERROR   stat=%x\n",*rdata);
			} 
		}

		for (x = 0; x < 32; ++x) {
		   if ((found = *(amd_rx_data+x)) != (exp = *(amd_tx_data+x))) {
		      error = TRUE;
		      if (!timeout && gp->g_option != 'N') {
			printf("error in receive buffer at %x  exp=%x obs=%x\n",
					    (int)(amd_rx_data+x),exp,found);
		      }
		   }
		}
	} /* end while mayget() */

  return(error);
}



#endif	M25

#ifdef	M25
/*
 * SCSI Interface Tests 
 */

scsi_test()

{
	char 	cmd;
	int	input, pass, wdata;
	u_long	vadrs;
	int	error, errors = 0;

	for (;;) {
		test_hdr("SCSI Interface", 0);
		display_opt("b", "SCSI Byte Ctr Wr/Rd"); 
                display_opt("c", "SCSI CS Reg Wr/Rd");
                display_opt("f", "FIFO/UDC DMA");
                display_opt("s", "SBC Chip Wr/Rd");
                display_opt("u", "9516 UDC Chip Wr/Rd");
		display_opt("x", "SCSI Bus External Loopback");
		if ((cmd = get_cmd()) == 'Q')   /* get command char */
			return (cmd);
		if (cmd != 'S' && cmd != 'U' && cmd != 'F' &&
		    cmd != 'X' && cmd != 'B' && cmd != 'C') {
			invalid_selmsg();
			continue;			/* Invalid command */
		}
		get_options();                          /* Get test options */ 
        	if (gp->g_option == 'Q')        
                	return('Q');                    /* quit test */
/*
 * Map in memory buffer space for testing AMD Ethernet chip and execute
 * test routine.
 */
 		/* First, setup segment map addresses for 16 pages, 1 entry */
 
		setsmreg(0x0f000000, 0xc0);	/* 16 pages */
		map(0xf000000, 16*PAGESIZE, ETHERMEM, PM_MEM); /*map 16 pages*/
		
		vadrs = (u_long) 0xf000000;

		/* Switch on cmd to cmd routine with loop control as */
		/* outer loop */
		errors = 0; 
		pass = 0;
		do {
			error =0;
			switch (cmd) {

			  case 'B':

			    error = byte_ctr_wr_rd_test();
			    break;

			  case 'C':

			    error = csr_reg_wr_rd_test();
			    break;

			  case 'F':

			    error = fifo_wr_rd_test(vadrs);
			    break;

			  case 'S':

			    error = scb_wr_rd_test();
			    break;

			  case 'U':

			    error = udc_wr_rd_test();
			    break;

			  case 'X':

			    error = scsi_loopback_test(); 
			    break;

			}	/* end switch */

			if (error != 0)
				++errors;

		} while (endtest(error, ++pass, errors));
	  }					/* end for */
}						/* end function */


/*
 * NCR SCSI 8350 Chip Write/Read Test  
 *
 *	This test will access the NCR SCSI chip's on board registers
 *	and test them as though they are memory registers.
 *
 *	The test consists of rotating a bit thru the 8 data lines into 
 *	NCR's Initiator Command Reg which is an 8 bit reg that most bits
 *	written to by the processor when the SCSI inactive. We will also
 *	use the Mode reg to cover the bits not testable on the ICR.
 *		Testing the address port will take a little more thought
 *	however the current plan is to arrange for various different
 *	values to be stored in the read/write and write/only registers
 *	to allow us to determine that we can access the individual registers
 *
 *
 *
 *	Proceedure:
 *		test data registers for all 8 bits
 *			First in initator command register to get bits 7,4-0.
 *			Bit 6 causes the NCR SCSI to tristate itself off
 *			the bus and can only be recovered by reset to chip
 *			thru the SCSI Control/Status word in the M25.
 *			Bit 5 causes trouble according to the hardware
 *			engineers and I haven't verified exactly what that
 *			problem is yet.
 *			Then in mode register to get bits 7-2,1,0.
 *			Bit 1 will only activate if bit 0 in initator command
 *			register is set so since we have tested that we seem
 *			to be able to access that register bit we should be
 *			able to get the full register here.
 *		see if we can reset the entire chip
 *			Here we will use the effect of bit 6 in the initator
 *			cmd reg to verify that we can reset the NCR chip.
 *		test the addresses available to the chip
 *
 */

int
scb_wr_rd_test()
{
  u_char found, exp, temp;
  u_short i, maxbits;
  int	  error;
  struct ncr_scsi_rd	  *scsi_rd;
  struct ncr_scsi_wr	  *scsi_wr;
  struct m25_scsi_control *scsi_ctl;


  error = FALSE;
  scsi_rd = (struct ncr_scsi_rd *)(u_int)SCSI_BASE;
  scsi_wr = (struct ncr_scsi_wr *)(u_int)SCSI_BASE;
  scsi_ctl = (struct m25_scsi_control *)((u_int)SCSI_BASE + M25_SCSI_CONTROL_OFFSET);

/*
 * Turn off reset to allow SCSI to run
 */

      scsi_ctl->status = scsi_ctl->status | M25_SCSI_RUN; /* remove reset */
      if (0)
	 error = TRUE;

/*
 * Test data lines with rotating bit             
 *
 * First in initiator_cmd reg which gets bits 7,4-0 
 */

      maxbits = (sizeof(scsi_rd->initiator_cmd) * 8);
      temp = 1;
      for (i = 0 ; i <= maxbits; i++)
	  {
	  exp= temp & (u_char)(~NCR_SCSI_TEST_MODE) /* remove the 2 bits that */
		    & (u_char)(~NCR_SCSI_DIFF_ENB); /* will cause trouble    */
	  scsi_rd->initiator_cmd = exp;
	  DELAY(25);		/* wait for bus to settle down */
	  if ((found = scsi_rd->initiator_cmd) != exp)
	      error = TRUE;
	  temp = temp << 1;
          }
/*
 * Turn on reset to remove any odd conditions caused by 
 * our bit twiddling. 
 */

      scsi_ctl->status = scsi_ctl->status & (~M25_SCSI_RUN); /* assert reset */
      if (0)
	 error = TRUE;

/*
 * And off again to test the next port
 */

      scsi_ctl->status = scsi_ctl->status | M25_SCSI_RUN; /* remove reset */
      if (0)
	 error = TRUE;
 
/*
 * Then in mode reg which gets bits 7-2,0
 *
 * Test mode register with walking 1                 
 */

      maxbits = (sizeof(scsi_rd->mode) * 8);
      temp = 1;
      for (i = 0 ; i <= maxbits; i++)
	  {
	  exp= temp & (~NCR_SCSI_DMA_MODE); /* remove the bit that   */
		    			    /* will cause trouble    */
	  scsi_rd->mode = exp;
	  DELAY(25);		/* wait for bus to settle down */
	  if ((found = scsi_rd->mode) != exp)
	      error = TRUE;
	  temp = temp << 1;
          }

/*
 * Turn on reset again to remove any odd conditions caused by 
 * our bit twiddling and to leave system in a consistant state
 * provided every thing is working (the way we think it should).
 */

      if (0)
	 error = TRUE;




 
/*
 * Test reg address port for normal range        
 */
 

  return(error);
}


/*
 * udc_wr_rd_test()   
 *
 *		This test will access the AMD Universal data controller chip's
 *   on board registers and test them as though they are memory registers.
 *
 *	The test consists of rotating a bit thru the 16 data lines into 
 *	AMD's current address Register lower address field which is a 16 bit
 *   reg that all bits can be read and written to by the processor
 *
 *	Proceedure:
 *		test address register for correct range of values
 *		Ask for CAR_LAF and rotate a bit thru the 16 data lines
 *		verify rotation OK.
 *
 *
 */

int
udc_wr_rd_test()
{
  u_short i, found, exp, maxbits;
  u_short *radrs, *rdata;
  int	  error;
  char str[128];

  error = FALSE;

  radrs = ((u_short *)SCSI_BASE)+AMD_UDC_PTR+AMD_UDC_OFFSET;
						/* set pointer to        */
						/* Register address port */
  rdata = ((u_short *)SCSI_BASE)+AMD_UDC_DATA+AMD_UDC_OFFSET;
					 /* set pointer to Register DATA */
 
/*
 * Test reg address port for normal range        
 */
 

  for (i = 0; i <= AMD_UDC_PTRMAX+1; i++) /* AMD might mask off values */ 
      {					/* greater than the max so we check */
      *radrs=i;
      DELAY(25);		/* wait for bus to settle down */
      if ( (found = *radrs) != (i & AMD_UDC_PTRMAX ) )
          error = TRUE;

      }

/*
 * Test data lines with rotating bit             
 */

      *radrs =  AMD_UDC_CAR_LAF_2; /* select Current Address Register */
				   /* Lower Address Field channel-2   */
      maxbits = (sizeof(*radrs) * 8);
      exp = 1;
      for (i = 0 ; i <= maxbits; i++)
	  {
	  *rdata = exp;
	  DELAY(25);		/* wait for bus to settle down */
	  if ((found = *rdata) != exp)
	      error = TRUE;
	  exp = exp << 1;
          }




  return(error);
}

/*
 * fifo_wr_rd_test(vadrs)
 *
 *			u_long vadrs;   defines the address in virtual
 *					memory to setup the control/data
 *					blocks
 *
 *		This test will access the fifo memory using the AMD Universal
 *   DMA controller chip.
 *
 *	The test consists of rotating a bit thru the 16 data lines into 
 *	the fifo which will test DMA access and 16 addresses of the fifo.
 *   We will then continue the test by doing an address test of the rest
 *	of the FIFO.
 *
 *	Proceedure:
 *		Setup to dma to the fifo from ram. (This means to map
 *		  some memory to the IO address space for UDC master access.)
 *		Dma to/from the fifo for data line test then address test.
 *		Check that it is correct.
 *
 *
 */


int
fifo_wr_rd_test(vadrs)
  u_long *vadrs;  /* where in virtual memory we are suposed to get/give	*/
		/* data as bus master					*/
{
  char fifo_empty;
  int all, error, errcnt;

  u_short i, j, found, exp, adrs, maxbits, result;

  u_short  *amd_rx_data;
  u_short  *amd_tx_data;

  char *mem_start;
  char *testptr;

  struct m25_scsi_control *scsi_ctl;

  struct amd_udc_table *udc_table;


  scsi_ctl = (struct m25_scsi_control *)((int)SCSI_BASE + M25_SCSI_CONTROL_OFFSET);

  mem_start = (char *)ETHERMEM;
  udc_table = (struct amd_udc_table *)vadrs;
  testptr = (char *)udc_table;
  amd_tx_data = (u_short *)vadrs+0x100;
  amd_rx_data = (u_short *)vadrs+0x200;
  setsmreg(udc_table, 0xc1);
  map(udc_table, PAGESIZE,mem_start,PM_MEM);

  error = FALSE;
/*
 * Test data lines with rotating bit             
 */

  scsi_ctl->status = scsi_ctl->status & (~M25_UDC_RUN)   & /*disable UDC  */ 
					(~M25_SCSI_SEND) & /*disable send */
					(~M25_FIFO_RUN);   /*disable fifo */

  scsi_ctl->status = scsi_ctl->status | M25_UDC_RUN |
					M25_FIFO_RUN;   /* enable Dma & fifo*/

  maxbits = (sizeof(*amd_tx_data) * 8);
  exp = 1;
  for (i = 0 ; i <= maxbits; i++) {

      scsi_ctl->status = scsi_ctl->status & (~M25_FIFO_RUN);/* disable */
								/* fifo    */

      scsi_ctl->status = scsi_ctl->status |	M25_FIFO_RUN;  /*enable fifo*/

      *amd_tx_data = exp; /* The value we expect to get back eventually */
	
      result = dma_tx(amd_tx_data,1,udc_table,INT_OFF); /* This word is */
							    /* ours to read */

      if ( result & M25_SCSI_SR_B_ERR )
          error = error | M25_SCSI_SR_B_ERR;
      if ( !(scsi_ctl->status & M25_SCSI_SR_FIFO_EMPTY) )
          error = error | ERROR_NO_FIFO_EMPTY;

      *amd_tx_data = 0x9249;/* Different from any value we may want  */
				/* because the FIFO pointers have been   */
				/* adjusted by the hardware to pass this */
				/* word on to the SCSI interface chip    */
				/* We will specifically check that later */
				/*is shipped on. (Hopefully to SCSI)     */

      result = dma_tx(amd_tx_data,1,udc_table,INT_OFF); /* this word is */
							    /* stored in    */
							    /* the FIFO and */
							    /* is needed by */
							    /* the hardware */
							    /* to allow us  */
							    /* to read the  */
							    /* first word   */
							    /* we sent.     */

      if ( result & M25_SCSI_SR_B_ERR )
          error = error | M25_SCSI_SR_B_ERR;
      if (scsi_ctl->status & M25_SCSI_SR_FIFO_EMPTY)
          error = error | ERROR_FIFO_EMPTY;


/*
 * Now Recieve from the fifo
 */

      *amd_rx_data = 0;

      result = dma_rx(amd_rx_data,2,udc_table);
      if ( result & M25_SCSI_SR_B_ERR )
          error = error | M25_SCSI_SR_B_ERR;



      if (((found = *amd_rx_data) != exp) && gp->g_option != 'N') {
          printf("F=%x  E=%x  rxd-0=%x -1=%x  txd=%x UDC Stat=%x  stat=%x\n",
								found,exp,
					                     *amd_rx_data,
							 *(amd_rx_data+1),
							     *amd_tx_data,
                                                                   result,
                                                         scsi_ctl->status);
	  error = TRUE;
      }
      exp = exp << 1;

  } 	/* end for */

/*
 * Test address lines by writing as many bits as 
 * we can (1 at a time) till we get fifo full    
 * then reading them back and comparing them     
 */

  scsi_ctl->status = scsi_ctl->status & (~M25_FIFO_RUN) &
					(~M25_UDC_RUN); /*reset fifo & UDC */


  scsi_ctl->status = scsi_ctl->status | M25_FIFO_RUN |
					M25_UDC_RUN;    /*enable fifo & UDC */


  adrs = 1;	/* starting data for write */
  while (adrs < 0x800) {

	/*fill fifo half way(its going to stop here any way)*/

      *amd_tx_data = adrs;
      dma_tx(amd_tx_data,1,udc_table,INT_OFF);
      adrs++;				 /* inc to next pattern */
  }
/*
 * now we start alternatly reading and writing for
 * 8k to test the entire ram address range
 */
  
  errcnt = 5;
  exp = 1;	/* starting data for read */
  *amd_rx_data = 0x55; /* remove the previous data once before checkin all */
  while (exp < 0x1000) {

 /* walk once around fifo from its ends */

      *amd_tx_data = adrs;
      dma_tx(amd_tx_data,1,udc_table,INT_OFF);
      dma_rx(amd_rx_data,1,udc_table);
      if ((found = *amd_rx_data) != exp) {
	  if (errcnt && gp->g_option != 'N') {
              printf("F=%x  E=%x\n",found,exp);
              errcnt--;
          }
          error = 2;
      }
      adrs++;				 /* inc to next pattern */
      exp++;				 /* inc to next pattern */
  }

/*
 * finally we try to empty the fifo
 */
  errcnt = 5;
  fifo_empty = FALSE;
  while ((exp < 0x2000) && (!fifo_empty)) {

 /* finish emptying the fifo */

      dma_rx(amd_rx_data,1,udc_table);
      if ((found = *amd_rx_data) != exp) {
	  if (errcnt && gp->g_option != 'N') {
              printf("F=%x  E=%x\n",found,exp);
              errcnt--;
          }
          error = 3;
      }	
      exp++;				 /* inc to next pattern */
      if (scsi_ctl->status & M25_SCSI_SR_FIFO_EMPTY)
	  fifo_empty = TRUE;
  }
  if ( result & M25_SCSI_SR_B_ERR )
          error = error | M25_SCSI_SR_B_ERR;
 
  return(error);
}


/*
 * rtn = dma_tx(tx_data,count,udc_table,interrupt)
 *			u_short *tx_data;   pointer to a block of WORDS
 *					    in memory to be transmitted to
 *					    the fifo.
 *
 *			u_short count;      Number of WORDS to be xmitted
 *					    (0= No DMA just effect interrupts)
 *
 *	struct amd_udc_table *udc_table;    This is the structure of the
 *					    control bytes in the memory
 *					    table provided by the caller
 *					    The size and address are fixed
 *					    an the caller is expected to
 *					    provide the location and 
 *					    enough space.  Also the location
 *					    is restricted to f000000 and up
 *
 *			char interrupt	    Enable/Disable interrupts from
 *					    chip. 0 = disable else enable
 * 					    (If dma is NOT wanted after
 *					    this action then set count = 0.)
 *
 *			u_short rtn	    The contents of the AMD UDC status
 *					    register.
 *
 *
 *		This routine sets up and executes a dma transfer to the fifo
 *   ram using the DMA controller chip.
 *
 *
 *	Proceedure:
 *		Setup to dma to the fifo from system ram.
 *		Dma to fifo
 *		Check for errors (this not yet implemented)
 *
 *
 */

int
dma_tx(tx_data,count,udc_table,interrupt)
u_short *tx_data;    /* pointer to data to be shipped to FIFO */
u_short count;	     /* integer count of number of WORDS to shift */
struct amd_udc_table *udc_table;
char    interrupt;
{
  u_short *radrs, *rdata;
  struct m25_scsi_control *scsi_ctl;



  scsi_ctl = (struct m25_scsi_control *)((int)SCSI_BASE + M25_SCSI_CONTROL_OFFSET);

  radrs = ((u_short *)(int)SCSI_BASE)+AMD_UDC_PTR+AMD_UDC_OFFSET;
						/* set pointer to        */
						/* Register address port */
  rdata = ((u_short *)(int)SCSI_BASE)+AMD_UDC_DATA+AMD_UDC_OFFSET;
					 /* set pointer to Register DATA */

  scsi_ctl->status = scsi_ctl->status | M25_SCSI_SEND; /* set send mode */ 


  /* setup udc dma info */
  if (count) /* if count <>0 then attempt txfr else just set/clear CIE */
    {
    udc_table->haddr = (((u_long)tx_data >> 8) & 0x00ff0000) |
					 AMD_UDC_ADDR_INFO;
    udc_table->laddr = (u_short)tx_data;
    udc_table->hcmr = AMD_UDC_CMR_HIGH;
    udc_table->count = count;
    udc_table->rsel = AMD_UDC_RSEL_SEND;
    udc_table->lcmr = AMD_UDC_CMR_LSEND;

  /* initialize chain address register */

    DELAY(SC_UDC_WAIT);
    *radrs = AMD_UDC_ADR_CAR_HIGH;
    DELAY(SC_UDC_WAIT);
    *rdata = ((int)udc_table & 0xff0000) >> 8;
    DELAY(SC_UDC_WAIT);
    *radrs = AMD_UDC_ADR_CAR_LOW;
    DELAY(SC_UDC_WAIT);
    *rdata = (int)udc_table & 0xffff;

  /* initialize master mode register */

    DELAY(SC_UDC_WAIT);
    *radrs = AMD_UDC_ADR_MODE;
    DELAY(SC_UDC_WAIT);
    *rdata = 0;
    DELAY(SC_UDC_WAIT);
    *rdata = AMD_UDC_MODE;

  /* issue start chain command */
    DELAY(SC_UDC_WAIT);
    DELAY(SC_UDC_WAIT);
    *radrs = AMD_UDC_ADR_COMMAND;
    DELAY(SC_UDC_WAIT);

    if (interrupt)
        {
        *rdata = AMD_UDC_CMD_SET_CIE;  /* enable interrupts */
        DELAY(SC_UDC_WAIT);
        *rdata = AMD_UDC_CMD_STRT_CHN; /* Start chain operation */
        DELAY(SC_UDC_WAIT);
        }
    else
        {
        *rdata = AMD_UDC_CMD_CLR_CIE;   /* disable interrupts */
        DELAY(SC_UDC_WAIT);
        *rdata = AMD_UDC_CMD_STRT_CHN; /* Start chain operation */
        }


    DELAY(SC_UDC_WAIT);
    *radrs = AMD_UDC_ADR_COMMAND;

    DELAY(count); /* wait COUNT number of microsec to let dma finish */

    DELAY(SC_UDC_WAIT);
    return(*rdata);
    }
  else /* if the count =0 then just change interrupts */
    {
 /* issue set/clear interrups (CIE) only */

    DELAY(SC_UDC_WAIT);
    *radrs = AMD_UDC_ADR_COMMAND;
    DELAY(SC_UDC_WAIT);

    if (interrupt)
        *rdata = AMD_UDC_CMD_SET_CIE;
    else
        *rdata = AMD_UDC_CMD_CLR_CIE;

    DELAY(SC_UDC_WAIT); /* just incase count =0 */
    return(*rdata);
    }


}


/*
 * dma_rx(rx_data,count,udc_table)
 *			u_short *rx_data;   pointer to a block of WORDS
 *					    in memory to be recieved from
 *					    the fifo.
 *
 *			u_short count;      Number of WORDS to be rcvd
 *
 *	struct amd_udc_table *udc_table;    This is the structure of the
 *					    control bytes in the memory
 *					    table provided by the caller
 *					    The size and address are fixed
 *					    an the caller is expected to
 *					    provide the location and 
 *					    enough space.  Also the location
 *					    is restricted to f000000 and up
 *
 *			u_short rtn	    The contents of the AMD UDC status
 *					    register.
 *
 *		This routine sets up and executes a dma transfer from fifo
 *   ram using the DMA controller chip.
 *
 *
 *	Proceedure:
 *		Locate SCSI controll logic including theAMD UDC chip.
 *						 (used to access the fifo)
 *		Setup to dma from the fifo to system ram.
 *		Dma from fifo
 *		Check for errors (this not yet implemented)
 *
 *
 */


int
dma_rx(rx_data,count,udc_table)
u_short *rx_data;    /* pointer to data to be shipped to FIFO */
u_short count;	     /* integer count of number of WORDS to shift */
struct amd_udc_table *udc_table;
{
  u_short *radrs, *rdata;
  struct m25_scsi_control *scsi_ctl;




  scsi_ctl = (struct m25_scsi_control *)((u_int)SCSI_BASE + M25_SCSI_CONTROL_OFFSET);

  radrs = ((u_short *)SCSI_BASE)+AMD_UDC_PTR+AMD_UDC_OFFSET;
						/* set pointer to        */
						/* Register address port */
  rdata = ((u_short *)SCSI_BASE)+AMD_UDC_DATA+AMD_UDC_OFFSET;
					 /* set pointer to Register DATA */

  scsi_ctl->status = scsi_ctl->status & (~M25_SCSI_SEND); /* set rcv mode */ 


  /* setup udc dma info */

  udc_table->haddr = ((u_long)rx_data >> 8) & 0x00ff0000 | 
					AMD_UDC_ADDR_INFO;
  udc_table->laddr = (u_short)rx_data;
  udc_table->hcmr = AMD_UDC_CMR_HIGH;
  udc_table->count = count;
  udc_table->rsel = AMD_UDC_RSEL_RECV;
  udc_table->lcmr = AMD_UDC_CMR_LRECV;

  /* initialize chain address register */

  DELAY(SC_UDC_WAIT);
  *radrs = AMD_UDC_ADR_CAR_HIGH;
  DELAY(SC_UDC_WAIT);
  *rdata = ((int)udc_table & 0xff0000) >> 8;
  DELAY(SC_UDC_WAIT);
  *radrs = AMD_UDC_ADR_CAR_LOW;
  DELAY(SC_UDC_WAIT);
  *rdata = (int)udc_table & 0xffff;

  /* initialize master mode register */

  DELAY(SC_UDC_WAIT);
  *radrs = AMD_UDC_ADR_MODE;
  DELAY(SC_UDC_WAIT);
  *rdata = AMD_UDC_MODE;

  /* issue start chain command */

  DELAY(SC_UDC_WAIT);
  *radrs = AMD_UDC_ADR_COMMAND;
  DELAY(SC_UDC_WAIT);
  *rdata = AMD_UDC_CMD_STRT_CHN;

  DELAY(count); /* wait COUNT number of microsec to let dma finish */
  DELAY(SC_UDC_WAIT);
  return(*rdata);
}

/*
 * 	byte_ctr_wr_rd_test()   
 *
 *		This test will access m25 scsi byte counter register
 *	and test it as though it is a memory register.
 *
 *	The test consists of rotating a bit thru the 16 data lines into 
 *	the byte counter which will verify that we have full access to 
 *	it.
 *
 *
 *	Proceedure:
 *		test registers for all 16 bits
 *
 */

int
byte_ctr_wr_rd_test()
{
  u_short found, exp;
  u_short i, maxbits;

  int	  error;

  struct m25_scsi_control *scsi_ctl;


  error = FALSE;

  scsi_ctl = (struct m25_scsi_control *)((int)SCSI_BASE + M25_SCSI_CONTROL_OFFSET);

/*
 * Test data lines with rotating bit             
 */


  maxbits = (sizeof(scsi_ctl->byte_cntr) * 8);
  exp = 1;
  
  for (i = 0 ; i <= maxbits; i++) {
      scsi_ctl->byte_cntr = exp;
      DELAY(25);		/* wait for bus to settle down */
      if ((found = scsi_ctl->byte_cntr ) != exp) {
	  error = TRUE;
	  if (gp->g_option != 'N')
              printf("exp=%x   found=%x addr=%x\n",exp,found,
					 &scsi_ctl->byte_cntr);
      }

      exp = exp << 1;
  }

  return(error);
}
/*
 * csr_reg_wr_rd_test()   
 *
 *		This test will access m25 scsi control/status register
 *	and test it as though it is a memory register.
 *
 *	The test consists of rotating a bit thru the 16 data lines into 
 *	the byte counter which will verify that we have full access to 
 *	it.
 *
 *
 *	Proceedure:
 *		test registers for all 4 bits
 *
 */

int
csr_reg_wr_rd_test()
{
  u_short found, exp, temp;
  u_short i, maxbits;

  int	  error;

  struct m25_scsi_control *scsi_ctl;


  error = FALSE;

/* locate the M25 dependent SCSI control logic */

  scsi_ctl = (struct m25_scsi_control *)((int)SCSI_BASE + M25_SCSI_CONTROL_OFFSET);


/*
 * Test data lines with rotating bit             *
 */


  maxbits = (sizeof(scsi_ctl->status) * 8);
  temp = 1;
  for (i = 0 ; i <= maxbits; i++) {
      exp= temp & (0xF); /* remove unused bits */
      scsi_ctl->status = exp;
      DELAY(25);		/* wait for bus to settle down */
      if ((found = (scsi_ctl->status & 0xFF) ) != exp)
	  error = TRUE;
      temp = temp << 1;
  }

  return(error);
}

/*
 * scsi_loopback_test()  
 *
 *	This test uses the control output signals onto the SCSI bus
 * as outputs to the SCSI bus and the SCSI bus data lines DB0-DB7
 * as inputs in order to test the NCR 5380 SCSI bus paths to the 
 * SCSI bus connector at the handle edge of the Model 50 CPU board
 * in order to perform a loopback test of the SCSI bus connector.
 *
 * 	In order to do this, the following SCSI bus connector pins
 * must be shorted together by means of a shorting connector which 
 * must be placed on the SCSI bus connector at the handle edge of 
 * the Model 50 CPU board during the test.
 *
 * SCSI Loopback Connector Wirelist
 *
 *	From    Chip  Conn	Conn   To     Chip
 *	Signal	Pin   Pin       Pin    Signal Pin
 *	------	---   ---       ------	---  ---
 *	ACK	14    38 ------ 16      DB7	2
 *	BSY	13    36 ------ 14      DB6	3
 *	SEL	12    44 ------ 12      DB5	4
 *	ATN     15    32 ------ 10      DB4	5
 *	REQ	20    48 ------ 8       DB3	6
 *	MSG	19    42 ------ 6       DB2	7
 *	C/D	18    46 ------ 4       DB1	8
 *	I/O	17    50 ------ 2       DB0	9
 *      RST     16    40 ------ 18      DBP     10 
 *
 */

int
scsi_loopback_test()
{
  u_char found, exp, temp;
  u_short i, maxbits;
  int	  error;
  struct ncr_scsi_rd	  *scsi_rd;
  struct ncr_scsi_wr	  *scsi_wr;
  struct m25_scsi_control *scsi_ctl;


  error = FALSE;
  scsi_rd = (struct ncr_scsi_rd *)(u_int)SCSI_BASE;
  scsi_wr = (struct ncr_scsi_wr *)(u_int)SCSI_BASE;
  scsi_ctl = (struct m25_scsi_control *)((u_int)SCSI_BASE + M25_SCSI_CONTROL_OFFSET);
/*
 * Turn off reset to allow SCSI to run
 */

      scsi_ctl->status = scsi_ctl->status | M25_SCSI_RUN; /* remove reset */
/* 
 * With no SCSI bus control signals asserted all SCSI data bus
 * inputs should be 0 (high).  Read the SCSI bus and verify that 
 * DB7-DB0 are 0 (high).: 
 */	

      if ((found = scsi_rd->current_data) != 0) {
	error = TRUE;
        if (gp->g_option != 'N') {
	  printf("SCSI Loopback Error: all signals shld be off(high) ");
	  printf("exp 0, obs %x\n", found);
	}
      }
      
/*
 * Turn on reset to reset SCSI control signals. 
 */

      scsi_ctl->status = 0;	/* reset NCR 8350 chip */ 
/*
 * Remove chip reset on NCR 8350 then load the cmd initiator register to set
 * 	ACK(#38)->DB7(#16) = LOW (1)
 *	BSY(#36)->DB6(#14) = LOW (1)
 *	SEL(#44)->DB5(#12) = LOW (1)
 *	ATN(#32)->DB4(#10) = LOW (1)
 *	REQ(#48)->DB3(#8)  = HI  (0) 
 *	MSG(#42)->DB2(#6)  = HI  (0)
 *	C/D(#46)->DB1(#6)  = HI  (0)
 *	I/O(#50)->DB0(#2)  = HI  (0)
 * 
 * output control signals low:  ACK(#38) ATN(#32) SEL(#44) BSY(#36)
 * output control signals high: REQ(#48) C/D(#46) MSG(#42) I/O(#50)
 * input data signals low:      DB7(#16) DB4(#10) DB5(#12) DB6(#14)
 * input data signals high:	DB3(#8)  DB1(#4)  DB2(#6)  DB0(#2)
 */
 
/*
 * Turn off reset to allow SCSI to run
 */

      scsi_ctl->status = scsi_ctl->status | M25_SCSI_RUN; /* remove reset */

	/* load inititor command register, set ACK,ATN,SEL,BSY only =1 */

      scsi_wr->initiator_cmd = NCR_SCSI_ASSERT_ACK + NCR_SCSI_ASSERT_ATN +
	NCR_SCSI_ASSERT_BSY + NCR_SCSI_ASSERT_SEL; 
      DELAY(10);

	/* then read current SCSI bus register to read state of looped */
	/* back control to data bus signals */

      found = scsi_rd->current_data;
      if (found != (exp = NCR_DB7 + NCR_DB4 + NCR_DB5 + NCR_DB6)) {
	error = TRUE;
	if (gp->g_option != 'N') {
	  printf("SCSI Loopback Error: Signals:ACK,BSY,SEL,ATN,REQ,MSG,");
	  printf("C/D,I/O:");
	  printf(" exp %x, obs %x\n", exp, found);
	}
      }
 
/*
 * Load the data output register with bits DB3,DB2,DB1,DB0 = 1(low) and
 * the reset of the data output bits = 0(hi).  Then read the state of the 
 * SCSI bus status register.  SCSI bus control signals: REQ,MSG,C/D, and,
 * I/O shld be read as 1 (low) and control signals ACK,ATN,SEL, and BSY
 * = 0(hi).
 * input control signals high:  ACK(#38) ATN(#32) SEL(#44) BSY(#36)
 * input control signals low:   REQ(#48) C/D(#46) MSG(#42) I/O(#50)
 * output data signals high:    DB7(#16) DB4(#10) DB5(#12) DB6(#14)
 * output data signals low:     DB3(#8)  DB1(#4)  DB2(#6)  DB0(#2)
 */

/*
 * Turn on reset to reset SCSI control signals. 
 */

      scsi_ctl->status = 0;	/* reset NCR 8350 chip */ 

/* Delay for reset to occur */

      DELAY(10);

/*
 * Turn off reset to allow SCSI to run
 */
 
      scsi_ctl->status = scsi_ctl->status | M25_SCSI_RUN; /* remove reset */
    
 
/*
 * Load Output Data Register with data bits 3,2,1,0 only 1(low)
 */
      scsi_wr->output_data = 0xF; 


/* Assert data bus bits 3,2,1,0 which return to REQ,MSG,C/D, and I/O. */

      scsi_wr->initiator_cmd = NCR_SCSI_ASSERT_DATA_BUS;
      DELAY(10);

/* Read current SCSI bus (control signal) status */

      found = 0;
      for (i = 0x40; i > 0; --i) {
	found = found | scsi_rd->current_bus_status;
      }

      if (found != (exp = 0x3D)) {
	error = TRUE;
	if (gp->g_option != 'N') {
	  printf("SCSI Loopback Error: Signals:RST,BSY,MSG,C/D,I/O,");
	  printf("SEL,DBP:");
	  printf(" exp %x, obs %x\n", exp, found);
	}
      }
/*
 * Turn on reset to reset SCSI NCR chip. 
 */

      scsi_ctl->status = 0;		/* assert chip reset */
      return (error);
}

#endif	M25

/* Serial Ports Test */

ports_test(mouse)
	int	mouse;
{
    register struct sptab *sp;
    int l, j, baud, error, errors, pass;
    int input;
    char *p_SCC_ctl, *p_SCC_data, port, cmd;
    char rdata, pattn = 'W', rstatus;

    if (mouse) {
	baud = 0x1200;
	port = 'M';
    } else {
	baud = 0x9600;
	port = 'B';
    }
    for(;;) {
	input = 0;
	while (input == 0) {			/* command input loop */
		input = 1;
		if (mouse)
			test_hdr("Mouse/Keyboard Ports", 1);
		else
			test_hdr("Serial Ports", 1);
                printf("Enter port cmd: Cmd [port(%s)]",
                        mouse ? "M or K":"A or B");
                printf(" [Baud rate(decimal #)] [hex byte pattern]\n\n");
		printf("Cmd -  Test\n\n");
                display_opt("w", "Wr/Rd SCC Reg 12");
		display_opt("x", "Xmit Char");
                display_opt("i", "Internal Loopback");
                display_opt("e", "External Loopback");
		if ((cmd = get_cmd()) == 'Q')   /* get command char */
			return (cmd);
                skipblanks();   /* skip spaces */ 

		if (peekchar() == '\r') {
			pattn = ++pattn;
                        break;                  /* Use default parameters? */
		}

		port = getone() & UPCASE;	/* get port */

		if ((mouse == 0 && port != 'A' && port != 'B') ||
		  (mouse != 0 && port != 'M' && port != 'K')) {
			invalid_selmsg();
			input = 0;
			continue;
		}
		if (((gp->g_insource == INUARTA) && (port == 'A')) ||
			((gp->g_insource == INUARTB) && (port == 'B'))) {
			printf("\nCannot test port in use!\n");
			input = 0;	/* input error-stay in prompt loop */
		}
                skipblanks();   /* skip spaces */ 
		if ((cmd == 'X') || (cmd =='I') || (cmd =='E')) {
			baud = getnum();		/* get Baud rate */
                        skipblanks();   /* skip spaces */ 
			pattn = (char) getnum();	/* get data pattern */
		} else if (cmd == 'W') {
			pattn = (char) getnum();  /* get byte pattern not */
		}
	}

	get_options();                          /* Get test options */ 
        if (gp->g_option == 'Q')        
                return('Q');                    /* quit test */

	if (port == 'A') {
                        p_SCC_ctl = (char *)SERIAL0_BASE + 4;
                        p_SCC_data = (char *)SERIAL0_BASE + 6;
        } else if (port == 'B') {
                        p_SCC_ctl = (char*)SERIAL0_BASE;
                        p_SCC_data = (char *)SERIAL0_BASE + 2;
        } else if (port == 'K') {
                        p_SCC_ctl = (char *)KEYBMOUSE_BASE + 4;
                        p_SCC_data = (char *)KEYBMOUSE_BASE + 6;
        } else {
                        p_SCC_ctl = (char *)KEYBMOUSE_BASE;
                        p_SCC_data = (char *)KEYBMOUSE_BASE + 2;
	} 


	/* First, reset the channel to be tested */

	reset_uart(p_SCC_ctl,0);

	/* Setup the time constant register for Baud rate */

	for (sp = speedtab;; sp++) {
		if (sp->speeds == baud) {
			*p_SCC_ctl = 12; /* WR 12, time const */
			DELAY(10);	 /* time delay */
			*p_SCC_ctl = sp->counts;
			DELAY(10);	 /* time delay */
			*p_SCC_ctl = 13; /* select WR 13 */
			DELAY(10);
			*p_SCC_ctl = sp->counts >> 8; 
			break;
		}
		if (sp->speeds == 0) {
			printf("Invalid Baud rate!\n");
			input = 0;
			break;
		}
	}
	errors = 0;
	pass = 0;
	switch (cmd) {		/* switch on command character */

		case 'E':			/* external loopback test */
			do {	
			  if (!(rstatus = scc_wait(p_SCC_ctl, ZSRR0_RX_READY)))
				rdata = *p_SCC_data;
			} while (rstatus == 0);

			wr_rd_char(pattn, port, p_SCC_ctl, p_SCC_data);
	    		break;

		case 'I':			/* internal loopback test */

		/* Enable local loopback in WR_14 of SCC chip */
	    		*p_SCC_ctl = 14; /* select WR 14, for loopback */
			DELAY(10);			/* time delay */
			*p_SCC_ctl =ZSWR14_BAUD_ENA| ZSWR14_BAUD_FROM_PCLK|
			    ZSWR14_LOCAL_LOOPBACK;
		
		/* enable local loopback */ 

		/* purge the receiver input FIFO */
		
			do {	
			  if (!(rstatus = scc_wait(p_SCC_ctl, ZSRR0_RX_READY)))
				rdata = *p_SCC_data;
			} while (rstatus == 0);

	    		wr_rd_char(pattn, port, p_SCC_ctl, p_SCC_data);
		 
		/* Disable local loopback */
	    	
			*p_SCC_ctl = 14;	/* select WR 14, for loopback */
			DELAY(10);			/* time delay */
			*p_SCC_ctl =ZSWR14_BAUD_ENA| ZSWR14_BAUD_FROM_PCLK;
			break;

		case 'W':			/* write/read RR12 of SCC */
			rstatus = *p_SCC_ctl; /* read to init */
			DELAY(10);
			do {
		 	  error = 0;
	        	  *p_SCC_ctl = 12;	/* select WR 12 of SCC */
	        	  DELAY(10);				/* time delay */
	        	  *p_SCC_ctl = pattn;	/* write data */
	       	 	  DELAY(10);				/* time delay */
	        	  *p_SCC_ctl = 12; /* select RR 12 of SCC */
			  DELAY(10)
	        	  rdata = *p_SCC_ctl;	/* read data */
	        	  if (pattn != rdata) {
		  	      error = scc_derr("", port, pattn, rdata);
			  }
			  if (error != 0) 
				++errors;

			} while (endtest(error, ++pass, errors));
	        break;

	  	case 'X':			/* Transmit character loop */

	    	do {
			error = 0;
			rstatus = scc_wait(p_SCC_ctl, ZSRR0_TX_READY);
			if (rstatus > 0) {
		  		error = scc_errmsg("TX", port, rstatus);
			}
			*p_SCC_data = pattn;		/* xmit character */

			if (error != 0)
				++errors;

		} while (endtest(error, ++pass, errors));
	    	break;
	}				/* end switch */	
    }					/* end for */
}					/* end function */

wr_rd_char(pattn, port, p_SCC_ctl, p_SCC_data)

	char pattn, port;
	char *p_SCC_ctl, *p_SCC_data;
{
	int  pass = 0, error, errors = 0; 
	char rdata, rstatus;

	do {
		error = 0;
		rstatus = scc_wait(p_SCC_ctl, ZSRR0_TX_READY);	

		if (rstatus > 0) {
			error = scc_errmsg("TX", port, rstatus);
		}

		*p_SCC_data = pattn;		/* xmit data pattern */
		rstatus = scc_wait(p_SCC_ctl, ZSRR0_RX_READY);	

		if (rstatus > 0) {
			error = scc_errmsg("RX", port, rstatus);
		}

		rdata = *p_SCC_data;	/* read Rx data */
		if (rdata != pattn)  {
			error = scc_derr("TX/RX ", port, pattn, rdata);
		}

		if (error != 0)
			++errors;

		} while (endtest(error, ++pass, errors));
}

/*
 *	Generic SCC wait routine.  Condition is set to either Rx or Tx
 */

scc_wait(p_SCC_ctl, cond)

	char 	*p_SCC_ctl;
	char	cond;
{
	char rstatus;
	int cnt = 100000;


	while (--cnt) {
	  	DELAY(10);			/* time delay for chip */
	  	rstatus = *p_SCC_ctl;		/* read SCC RR 0, status */
	  	if ((rstatus & cond) > 0) 
			return(0);
	}
	return (rstatus);
}
/* Keyboard Test
 * Reads typed input from keyboard and outputs it to the output display
 * device specified by the calling argument.
 */

keybd_test()
{
	char character, rstatus;
	char  *p_SCC_ctl;
	int save_channel;

	printf("\nType keyboard keys to display them.  To exit type 'esc'.\n");
	gp->g_echo = 0;			/* turn off echo */
	if (gp->g_insource > 0) {
          	if (gp->g_insource == INUARTA) 
	    		p_SCC_ctl = (char *)SERIAL0_BASE + 4;
	  	else
	    		p_SCC_ctl = (char *)SERIAL0_BASE;
	  	save_channel = gp->g_insource;
	  	gp->g_insource = INKEYB;	

	  	for (;;) {
	    		rstatus = *p_SCC_ctl & ZSRR0_RX_READY;
	    		if (rstatus != 0) 
				break;
	    		character = getchar();
	    		printf("  ");
	    		printhex((int)character, 2);
	    		if (character > ' ' && character < 0x7F)
	      			printf("  %c\n", character);
	  	}
	  	gp->g_insource = save_channel;
	}
	else {	/* Sun keyboard test */

#ifdef GRUMMAN	/* give warning in case got in here somehow. */
		printf("**** ERROR: testing a Sun keyboard ****\n");
#endif GRUMMAN
	  	for (;;) {
	      		character = getchar();
	      		if (character > 0 && (character & UPCASE) == ESC_ASCII)
				break;
	      		printf("  ");
	      		printhex((int)character, 2);
	      		if (character > ' ' && character < 0x7F) 
	      			printf("  %c\n", character);
	  	}
	}
	gp->g_echo = 1;
}	/* end of keybd_test() */


#ifndef PRISM
/*
 * For readability, ifdef separate memory_test for prism.
 */
/*
 * Memory & Video Test 
 */

memory_test(space, video)
    	int space, video;
{
    	register int address;
    	int 	lower, upper, pattern, rdata;
    	int 	error, errors = 0, base, pass, feedback = 0;
    	char 	cmd;
    	char    *ind = "-\\|/";
	
    	for(;;) {                    /* command input loop */
		pass = 0;
		if (video) {            /* Video test parameters */
			lower = (int)VIDEOMEM_BASE;
			upper = (int)VIDEOMEM_BASE + VIDEOMEM_SIZE - 1; 
			pattern = 0xFF00FF00; 
       		} else {                /* Main memory test */ 
			lower = 0;
			upper = gp->g_memoryavail - 1;  
			if (upper > 0x7fffff)
				upper = 0x7fffff;
			pattern = 0xAAAAAAAA;   
		}
		base = lower;

		if (video)
			test_hdr("Video", 1);
		else
			test_hdr("Memory", 1);

		printf("Enter Cmd [low addr > 0] [hi addr < 0x%x] [pattn]\n\n", 
			upper - base);
		printf("Cmd -  Test\n\n");
        	display_opt("a", "Address");
        	display_opt("c", "Data Compare");
		display_opt("s", "Scan Memory"); 
        	display_opt("w", "Write Only");

		if ((cmd = get_cmd()) == 'Q') {   /* get command char */
			gp->g_loop = 0x0;
			return(cmd);
		}

		if (cmd != 'A' && cmd != 'C' && cmd != 'S' && cmd != 'W') {
			invalid_selmsg();
			continue;
		}

        	skipblanks();   /* skip spaces */ 
		if (peekchar() != '\r') {
			lower = base + getnum();  /* get beginning address */
        		skipblanks();   /* skip spaces */ 
			upper = base + getnum();	/* get ending address */
        		skipblanks();   /* skip spaces */ 
			pattern = getnum();      	/* get hex pattern */
    		}					

 		get_options();                          /* Get test options */ 
        	if (gp->g_option == 'Q') {       
			gp->g_loop = 0x0;
                	return('Q');                    /* quit test */
		}

		printf("\n%s Test from [0x%x - 0x%x].\n\n", 
			video == 1? "Video" : "Memory", lower, upper);

		upper = upper & 0xFFFFFFFC;

		if (gp->g_option == 'F' || gp->g_option == 'L' || 
		    gp->g_option == 'N') 
			gp->g_loop = 0x01;
		do {
		    error = 0;
		    if (cmd != 'S') {
			for (address = lower; address <= upper; address += 4) {
				if (cmd == 'A')
					pattern = address;
				putsl(address, space, pattern);
		   	 }
		    }

		    if (cmd != 'W') {
		   	for (address = lower; address <= upper; address += 4) {
			   	if (cmd == 'A')
			   		pattern = address;
				rdata = getsl(address, space);
#ifndef	M25
				if (rdata != pattern && cmd != 'S' &&
			       		gp->g_option != 'N') {
			       		printf("Data Error (%d):  Addr = 0x%x",
			               		++error, address - base);
			       printf(", Exp = 0x%x, Obs = 0x%x, Xor = 0x%x.\n",
				      		pattern, rdata, pattern^rdata);
			       		++errors;
			       		if (gp->g_option == 'H') {
				   		printf("Halted on error => hit any key to continue!\n");
				   		getchar();
			       		}
		 		}	/* if rdata!=pattern && .... */
#else
			if (rdata != pattern && !video && cmd != 'S' &&
			    (gp->g_option != 'N') && 
			    ((address < (int)VIDEO_BASE) ||
			    (address >= ((int)VIDEO_BASE + VIDEOMEM_SIZE)))) {
                               printf("Data Error (%d):  Addr = 0x%x",
                                       ++error, address - base);
                               printf(", Exp = 0x%x, Obs = 0x%x, Xor = 0x%x.\n",                                      pattern, rdata, pattern^rdata);
                               ++errors;
                               if (gp->g_option == 'H') {
                                   printf("Halted on error => ");
                                   printf("hit any key to continue!\n");
                                   getchar();
                               }
                           }
#endif	M25
			
				if (error && gp->g_option == 'L') {
					printf("Looping on error ...\n");
					address -= 4;
				}

				if (!video && (address % 0x10000) == 0 &&
				 gp->g_option != 'N')
					printf("%c\b", ind[feedback++ % 4]);
			}	/* for (address=lower ....) */
		}	/* if (cmd!='W') */
	    } while (endtest(error, ++pass, errors));
	}
}					/* end function memory_test */

#endif PRISM	/* the above is for all Sun3 models except prism. */


#ifdef PRISM

memory_test(space, test_type)
    int space,
	test_type;		/* 0: memory test;
				 * 1: monochrome frame buffer test
				 * 2: enable plane test
				 * 3: color frame buffer test
				 * 4: color map test
				 */
{
    register int address;
    int 	lower, upper, upper_limit, maxaddr, pattern, rdata;
    int 	error, base, errors = 0, input = 0, pass, feedback = 0;
    char 	cmd;
    char    *ind = "-\\|/";
    char	*testname; /* string for name of test. */ /* not good chr[] */

    for (;;) {
      input = 0;
      pass = 0;
      while (input == 0) {                    /* command input loop */
	input = 1;
	switch(test_type) {
	case 0:		/* memory test */
		lower = 0;
		upper = gp->g_memoryavail - 1;  
		pattern = 0xAAAAAAAA;
		testname = "Memory";
		break;
	case 1:		/* monochrome frame buffer test */
		lower = (int)VIDEOMEM_BASE;
		upper = (int)VIDEOMEM_BASE + VIDEOMEM_SIZE - 1; 
		pattern = 0xFF00FF00; 
		testname = "Monochrome Video";
		break;
	case 2:		/* enable plane test */
		lower = (int)BW_ENABLE_MEM_BASE;
		upper = (int)BW_ENABLE_MEM_BASE + VIDEOMEM_SIZE - 1;
		pattern = 0xFF00FF00;
		testname = "Enable Plane";
		break;
	case 3:		/* color frame buffer test */
		lower = (int)PRISM_CFB_BASE;
		upper = (int)PRISM_CFB_BASE + PRISM_CFB_SIZE - 1;
		pattern = 0xFF00FF00;
		testname = "Color Frame Buffer";
		break;
	case 4:		/* color map test */
		lower = (int)COLORMAP_BASE;
		upper = (int)COLORMAP_BASE + (3 * COLORMAP_SIZE) - 1;
		pattern = 0xFF00FF00;
		testname = "Color Map";
		break;
	default:
		printf("test_type %1d does not exist for memory_test()\n");
		return('Q');	/* return to calling routine (monitor()). */
				/* This case should never be called.  test_type
				 * is set only by the boot prom.  So this is
				 * just a precaution.
				 */
	}	/* end of switch(test_type) */

	base = lower;
	maxaddr = upper - base;

	printf("\n%s Tests:  (Enter 'q' to return to Test Menu)\n\n",
		testname);
        printf("Enter Cmd [low addr >= 0] [hi addr <= 0x%x]", maxaddr);
	printf(" [pattn]\n\n");
       	printf("Cmd - Test\n\n");
       	printf(" a  - Address Test\n");
	printf(" c  - Write/Read Test\n");
	printf(" s  - Scan Memory Test\n");
       	printf(" w  - Write Only Test\n");
	if (test_type==4)
		printf(" f  - Fill color maps with default pattern\n");
       	printf("\nCmd=>");

       	getline(1);                     /* get command line input */
        skipblanks();   /* skip spaces */ 

       	if ((cmd = getone() & UPCASE) == 'Q')	/* Get test selection */ 
			return(cmd);		/* exit memory_test() */
	
	if (test_type == 4 && cmd == 'F') {
		fill_colormaps();
		printf("\nFilled color maps.\n\n");
		input = 0;
		continue;
	}

	if (cmd != 'W' && cmd != 'A' && cmd != 'C' &&
            cmd != 'R' && cmd != 'S') {     /* Invalid command? */
		input = 0;
		invalid_selmsg();
		continue;
	}

        skipblanks();   /* skip spaces */ 
	if (peekchar() == '\r')
		break;			/* Use default parameters? */
	lower = base + getnum();	/* get beginning address */

        skipblanks();   /* skip spaces */ 
	upper = base + getnum();	/* get ending address */

        skipblanks();   /* skip spaces */ 
	pattern = getnum();      	/* get hex pattern */

	if (upper < lower || upper > (maxaddr + base)) {
		printf("Address range error: range 0 - %x.\n",maxaddr);
		input = 0;
		continue;
	}

      }		/* end while (input==0) */


	get_options();                          /* Get test options */ 
        if (gp->g_option == 'Q')        
                return('Q');                    /* quit test */
	printf("\n%s test from address 0x", testname); 
	printhex(lower, 8);
	printf(" to 0x");
	printhex(upper, 8);
	printf(".\n\n");
	upper_limit = upper & 0xFFFFFFFC;
	errors = error = 0;		/* Initialize error &  counter */
	pass = 0;			/* Initialize pass counter */
	feedback = 0;			/* ditto feedback indicator */
	if (gp->g_option == 'F' || gp->g_option == 'L' || gp->g_option == 'N') 
		gp->g_loop = 0x01;
	do {
		if ((test_type==3 || test_type==4) && cmd!='R' && cmd!='S')
			rect_enable(0, 0, SCRWIDTH, SCRHEIGHT, 0);
				/* enable color planes for color fb test or
				 * color map test
				 */
		if (cmd != 'R' && cmd != 'S') {
			for (address = lower; address <= upper_limit;
			 address += 4) {
				if (cmd == 'A')
					pattern = address;
				putsl(address, space, pattern);
					/* write to memory */
			}
		}

		if (cmd != 'W') {
			for (address = lower; address <= upper_limit;
			     address += 4) {
				if (cmd == 'A')
					pattern = address;
				rdata = getsl(address, space);
					/* read from memory */
				if (rdata != pattern && cmd != 'S' &&
				 gp->g_option != 'N') {
					 printf("Data compare error (%d):\n",
						 ++errors);  /* MJC */
					 printf("   addr = 0x");
					 printhex(address - base, 8);
					 printf(", exp = 0x");
					 printhex(pattern, 8);
					 printf(", obs = 0x");
					 printhex(rdata, 8);
					 printf(", xor = 0x");
					 printhex(pattern ^ rdata, 8);
					 printf(".\n");
					 if (gp->g_option == 'H') {
					   printf("Halted on error => ");
					   printf("hit any key to continue!\n");
					   getchar();
			       		 }
				 error = 1; /* set error to fail  MJC */
				}	/* if rdata!=pattern && .... */

				if (error && gp->g_option == 'L') {
					printf("Looping on error ...\n");
					address -= 4;
				}

				if (test_type==0 && (address % 0x10000) == 0 &&
				    gp->g_option != 'N')
					printf("%c\b", ind[feedback++ % 4]);
					/* indicate program running (every 64
					 * Kbytes).  For memory test only.
					 */
			}	/* for (address=lower ....) */
		}	/* if (cmd!='W') */
		if (gp->g_option != 'N') {
				/* if video test (e.g., test_type=1) and write,
				 * then clear the screen.
				 */
			if ((test_type==1 || test_type==3 || test_type==4) &&
					cmd != 'R' && cmd != 'S')
				fwritestr("\f", 1);
			printf("%s test completed pass %d with %d errors.\n",
				testname, ++pass, errors);  /* MJC errors */
			DELAY(1000000);
		}
	} while (endtest(error,pass,errors));
	gp->g_loop = 0x0;		/* turn off cont_on_err option */
    }					/* end for (;;) */
}					/* end function memory_test */


/*
 *  fill_colormaps fills in the red, green, and blue color maps with a default
 *  pattern.  The pattern is a linearly increasing value in a color map.
 *  Starting with the first location in the red map, we enter a 0, then a 1
 *  then a 2, until we get to the last location of the red map, where we
 *  enter a 0xFF.  We enter a similar pattern for the green map, except we
 *  enter a 0 starting at 2/3 of the way down in the map.  (The values
 *  entered will wrap around so that 0xFF will be in the location just
 *  before where 0 is entered.  The blue map has a similar pattern, except
 *  0 is entered at 1/3 way down in the map.
 */

fill_colormaps()
{
	unsigned char *mapaddr;		/* pointer to the color map */
	short i, j;

	mapaddr = (unsigned char *)COLORMAP_BASE;
	for (j=0; j<3; j++) {	/* do for all 3 maps */
		for (i = 0; i < COLORMAP_SIZE; i++) {  /* for each map, fill */
			*mapaddr = (i + (j * COLORMAP_SIZE/3)) % COLORMAP_SIZE;
			mapaddr++;
				/*
				 * increment pointer to next byte in
				 * in color map
				 */
		}
	}
}	/* end of fill_colormaps */

#endif PRISM

/*
 * Get_options routine gets test control option after presenting menu and
 * checking syntax.  Returns the option character: 'F','H', 'L', 'Q' or 0.
 */
get_options()
{
	for (;;) {
		printf("\nTest Options: (Enter 'q' to return to Test Menu)\n\n");
        printf("Cmd  -  Option\n\n");
		printf(" f   -  Loop forever\n");
		printf(" h   -  Loop forever with Halt on error\n");
		printf(" l   -  Loop once with Loop on error\n");
		printf(" n   -  Loop forever with error messages inhibited\n");
		printf("<cr> -  Loop once\n");
		printf("\nCmd=>");
		getline(1);
		gp->g_option = getone() & UPCASE;

		if (gp->g_option == 'F' || gp->g_option == 'H' || 
		    gp->g_option == 'L' || gp->g_option == 'Q' || 
	    	    gp->g_option == 'N' || gp->g_option == 0){
			printf("\n");
			return;		/* Valid command */
		}
		invalid_selmsg();
	}						/* end of for loop */
}

/*
 *      Displays test menu header
 */
test_hdr(tstmsg, tstinfo)
	char	*tstmsg;
	char	tstinfo;
{
	printf("\n%s Tests:  (Enter 'q' to return to Test Menu)\n\n", tstmsg);
	if (!tstinfo)
        	printf("Cmd -  Test\n\n");
}

/*
 *	Display an option string
 */

display_opt(optcmd, optmsg)
	char    *optcmd, *optmsg;
{
	printf("  %s - %s Test\n", optcmd, optmsg);
}

/*
 *	Display command prompt and wait for command
 */

get_cmd() {
	printf("\nCmd=>");
        getline(1);                             /* get command line */
        skipblanks();
	return(getone() & UPCASE);		/* return the command */
}

/*
 *	Displays messages when gp->g_option != 'N'
 */

display_msg(message)
	char	*message;
{
	if (gp->g_option != 'N')
		printf("%s", message);
}

/*
 *	Display Invalid Selection Message
 */

invalid_selmsg() {
	printf("\nInvalid Selection!\n");
}

/*
 * 	Set End Test flag if proper conditions are met
 */

endtest(error, pass, errors)
	int	error, pass, errors;
{
	if (gp->g_option != 'N') {
		printf("Test %sed during pass %d.  Total errors = %d.\n", 
			error == 0? "pass" : "fail", pass, errors);
	}

	if ((gp->g_option == 'F' || gp->g_option == 'N' || 
	    (gp->g_option == 'L' && error != 0) || 
	    (gp->g_option == 'H' && error == 0)) && (mayget() < 0))
		return(1);
	else
		return(0);
}

/*
 *	Display SCC timeout errors
 */

scc_errmsg(str, port, status)
	char	*str;
	char	port, status;
{
	if (gp->g_option != 'N') {
        	printf("%s ready timeout: Port = %c, SCC status = %x.\n",
                        str, port, status);
        }
	return(1);
}

/*
 *	Display SCC data compare errors
 */

scc_derr(str, port, pattn, rdata)
	char    *str;
        char    port, pattn, rdata;
{
	if (gp->g_option != 'N') {
            printf("%sData Error: Port = %c, Exp = %x, Obs = %x, Xor = %x.\n",
                    str, port, pattn, rdata, pattn^rdata);
        }
	return(1);
}

