/*	adreg.h	6.1	83/07/29	*/

struct addevice {
	short int ad_csr;			/* Control status register */
	short int ad_data;			/* Data buffer */
};

#define AD_CHAN		ADIOSCHAN
#define AD_READ		ADIOGETW
#define	ADIOSCHAN	_IOW(a, 0, int)		/* set channel */
#define	ADIOGETW	_IOR(a, 1, int)		/* read one word */

/*
 * Unibus CSR register bits
 */

#define AD_START		01
#define AD_SCHMITT		020
#define AD_CLOCK		040
#define AD_IENABLE		0100
#define AD_DONE 		0200
#define AD_INCENABLE		040000
#define AD_ERROR		0100000
