#ifndef ECBOOT
#ifndef lint
static	char sccsid[] = "@(#)if_ec.c	1.1 84/12/21	Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "saio.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../sunif/if_ecreg.h"
#include "../mon/sunromvec.h"
#include "../mon/s2addrs.h"

#define millitime() (*romp->v_nmiclock)

int ecxmit(), ecpoll(), ecreset();

unsigned int ecstd[] = { 0xE0000, 0xE2000, 0 };

struct saif ecif = {
	ecxmit,
	ecpoll,
	ecreset,
};

#define CSRSET(v) ec->ec_csr = (ec->ec_csr & EC_INTPA) | (v)
#define CSRCLR(v) ec->ec_csr = ec->ec_csr & (EC_INTPA & ~(v))

ecprobe()
{
	if (peek((short *)(MBMEM_BASE + ecstd[0])) != -1)
		return (0);
	return (-1);
}

ecopen(sip)
	struct saioreq *sip;
{
	sip->si_sif = &ecif;
	if (sip->si_ctlr < ((sizeof ecstd)/sizeof *ecstd) - 1)
		sip->si_devdata = (caddr_t)(MBMEM_BASE + ecstd[sip->si_ctlr]);
	else
		sip->si_devdata = (caddr_t)(MBMEM_BASE + sip->si_ctlr);
	ndinit();
	ecreset((struct ecdevice *)sip->si_devdata);
	return (ndopen(sip));
}


ecreset(ec)
	register struct ecdevice *ec;
{

	ec->ec_csr = EC_RESET;
	DELAY(20);
	/* FIXME: following won't work if it's the first call, since aram
	   will not get set. */
	localetheraddr(&ec->ec_arom, &ec->ec_aram);
	CSRSET(EC_AMSW);
	CSRSET(EC_PA | EC_ABSW | EC_BBSW);
}

ecxmit(ec, buf, count)
	register struct ecdevice *ec;
	caddr_t buf;
	int count;
{
	caddr_t cp;
	int time = millitime() + 500;	/* .5 seconds */
	short mask = -1, back;

	cp = (caddr_t)&ec->ec_abuf[-count];
	bcopy(buf, cp, count);
	*(short *)(ec->ec_tbuf) = cp - (caddr_t)ec->ec_tbuf;
	CSRSET(EC_TBSW);
	for (;;) {
		if (millitime() > time) {
			ecreset();
			return (-1);
		}
		if (ec->ec_csr & EC_JAM) {
			mask <<= 1;
			if (mask == 0) {
				ecreset();
				return (-1);
			}
			back = -(millitime() & ~mask);
			if (back == 0)
				back = -(0x5555 & ~mask);
			ec->ec_back = back;
			CSRSET(EC_JAM);
		}
		if ((ec->ec_csr & EC_TBSW) == 0)
			return (0);
	}
}

ecpoll(ec, buf)
	register struct ecdevice *ec;
	char *buf;
{
	short len, *sp;
	int xbsw = 0;

	if ((ec->ec_csr & EC_ABSW) == 0) {
		sp = (short *)ec->ec_abuf;
		xbsw = EC_ABSW;
	} else if ((ec->ec_csr & EC_BBSW) == 0) {
		sp = (short *)ec->ec_bbuf;
		xbsw = EC_BBSW;
	} else
		return (0);
	len = *sp++ & EC_DOFF;
	bcopy((char *)sp, buf, len);
	CSRSET(xbsw);
	return (len);
}
