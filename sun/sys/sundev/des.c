#ifndef lint
static	char sccsid[] = "@(#)des.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *  Des Chip driver - for AMD AmZ8068 (AMD9518)
 */
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/file.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/des.h" 
#include "../machine/pte.h"
#include "../sundev/desreg.h"
#include "../sundev/mbvar.h"

/*
 * Driver information for auto-configuration stuff.
 */
int	desprobe();
struct	mb_device *desinfo[1];
struct	mb_driver desdriver = {
	desprobe, 0, 0, 0, 0, 0,
	sizeof (struct deschip), "des", desinfo, 0, 0, 0,
};
struct	deschip *des_addr;

static struct buf desbuf;

/*
 * Input parameters to chip
 */
static u_char *dest_key;
static enum desmode dest_mode;
static enum desdir dest_dir;
static u_char *dest_ivec;


desprobe(reg)
	caddr_t reg;
{
	int i;
	register struct deschip *desbase = (struct deschip *)reg;
	register int c;

	if ((c = pokec((char *)&desbase->d_selector, DESR_CMD_STAT)) != 0)
		return (0);
	if ((c = pokec((char *)&desbase->d_reg, DESC_RESET)) != 0)
		return (0);
	for (i = 0; i < 100; i++) /* wait for reset */
		continue;
	if ((c = pokec((char *)&desbase->d_selector, DESR_MODE)) != 0)
		return (0);
	c = peekc((char *)&desbase->d_reg);
	if (c == -1)
		return 0;
	if ((c&0xf)  != DESM_MC_SE)
		return (0);
	des_addr = (struct deschip *)reg;
	return (sizeof (struct deschip));
}

/*ARGSUSED*/
desopen(dev, flag)
	dev_t dev;
	int flag;
{

	return (0);
}

/*ARGSUSED*/
desclose(dev, flag)
	dev_t dev;
	int flag;
{

	return (0);
}

/*ARGSUSED*/
desioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
	int cmd;
	int flag;
{
	register struct desparams *dp;

	switch (cmd) {

	case DESIOCBLOCK:
		dp = (struct desparams *)data;
		if ((dp->des_len % 8) != 0 || dp->des_len > DES_MAXLEN) {
			return (EINVAL);
		}
		dest_key = dp->des_key;
		dest_ivec = dp->des_ivec;
		dest_mode = dp->des_mode;
		dest_dir = dp->des_dir;
		return (desblock(dev, (u_char *)dp->des_buf, dp->des_len));

	case DESIOCQUICK:
		dp = (struct desparams *)data;
		if ((dp->des_len % 8) != 0 || dp->des_len > DES_QUICKLEN) {
			return (EINVAL);
		}
		dest_key = dp->des_key;
		dest_ivec = dp->des_ivec;
		dest_mode = dp->des_mode;
		dest_dir = dp->des_dir;
		desdoit((u_char *)dp->des_data, dp->des_len);
		return (0);
	
	default:
		return (EINVAL);
	}
}

/* 
 * Process a block of data.
 * This is very much like a driver read since
 * we modify the user's buffer.
 */
desblock(dev, addr, len)
	u_char *addr;
	u_int len;
{
	struct uio uio;
	struct iovec iov;
	int desstrategy();

	iov.iov_base = (caddr_t)addr;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_seg = UIOSEG_USER;
	uio.uio_resid = len;
	return (physio(desstrategy, &desbuf, dev, B_READ, minphys, &uio));
}

desstrategy(bp)
	register struct buf *bp;
{

	desdoit((u_char *)bp->b_un.b_addr, (u_int)bp->b_bcount);
	bp->b_resid = 0;
	iodone(bp);
}

/*
 * Do the real work
 */
desdoit(in, len)
	register u_char *in;
	u_int len;
{
	
	register unsigned mode;
	register enum desdir dir = dest_dir;
	register u_char *out, *iv;
	register struct deschip *desp = des_addr;
	register u_char *end;


#	define DO8(expr) \
		expr; expr; expr; expr; expr; expr; expr; expr;

	if (dest_mode == ECB) {
		mode = DESM_ECB | DESM_M_ONLY;
	} else {
		mode = DESM_CBC | DESM_M_ONLY;
	}


	/* 
  	 * set mode of DES chip
 	 */
   	desp->d_selector = DESR_MODE;	/* select mode reg */
	desp->d_reg = mode;
	DELAY(4);		/* !!! be nice if mode could be cached */

	/* 
	 * load DES key
	 */
	desp->d_selector = DESR_CMD_STAT;	/* select cmd reg */
	desp->d_reg = (dir == ENCRYPT) ? DESC_LOAD_E_KEY : DESC_LOAD_D_KEY;
	desp->d_selector = DESR_IO;
	iv = dest_key;
	DO8(desp->d_reg = *iv++);
	
	if (dest_mode == CBC) {
		/* 
		 * load input vector
		 */
		desp->d_selector = DESR_CMD_STAT;	/* select cmd reg */
		desp->d_reg = (dir == ENCRYPT) ? 
			DESC_LOAD_C_IVE : DESC_LOAD_C_IVD;
		desp->d_selector = DESR_IO;
		iv = dest_ivec;
		DO8(desp->d_reg = *iv++);
	}

	/*
	 * Start the engine
	 */
	desp->d_selector = DESR_CMD_STAT;	/* select cmd reg */
	desp->d_reg = (dir == ENCRYPT) ? DESC_START_ENC : DESC_START_DEC;


	/*
	 * Process all but the last block
	 */
	end = in + len - 8;
	out = in;
	while (in < end) {
		/*
 		 * Feed the DCP
		 */
		desp->d_selector = DESR_IO;
		DO8(desp->d_reg = *in++);

		/*
		 * Wait until it's done
		 */
		desp->d_selector = DESR_CMD_STAT;
		while (desp->d_reg & DESS_BUSY)
			;	

		/*
		 * Swallow the output 
		 */
		desp->d_selector = DESR_IO;
		DO8(*out++ = desp->d_reg);
	}

	/*
	 * Now do the last block, reading the iv's out in the process 
	 * if CBC mode. We weren't able to read the iv's out of the chip, 
	 * so we do it in software. 
	 */
	desp->d_selector = DESR_IO;		/* Feed */
	DO8(desp->d_reg = *in++);

	desp->d_selector = DESR_CMD_STAT; 	/* Wait */
	while (desp->d_reg & DESS_BUSY)
		;

	/* Swallow */		
	if (dest_mode == CBC) {
		if (dir == DECRYPT) {
			in -= 8;
			iv = dest_ivec;		/* next iv = last block in */
			DO8(*iv++ = *in++);		
		}
		desp->d_selector = DESR_IO;
		DO8(*out++ = desp->d_reg);
		if (dir == ENCRYPT) {
			out -= 8;
			iv = dest_ivec;
			DO8(*iv++ = *out++);	/* next iv = last block out */
		}
	} else {
		desp->d_selector = DESR_IO;
		DO8(*out++ = desp->d_reg);
	}	
}
