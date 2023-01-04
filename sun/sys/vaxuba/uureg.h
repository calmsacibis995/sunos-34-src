
/*	uureg.h	6.1	83/07/29	*/

/*
 * DL11-E/DL11-W UNIBUS (for TU58) controller registers
 */
struct uudevice {
	short	rcs;	/* receiver status register */
	short	rdb;	/* receiver data buffer register */
	short	tcs;	/* transmitter status register */
	short	tdb;	/* transmitter data buffer register */
};

/*
 * Receiver/transmitter status register status/command bits
 */
#define UUCS_DONE	0x80	/* done/ready */
#define	UUCS_READY	0x80
#define UUCS_INTR	0x40	/* interrupt enable */
#define	UUCS_MAINT	0x02	/* maintenance check (xmitter only) */
#define	UUCS_BREAK	0x01	/* send break (xmitter only) */

/*
 * Receiver data buffer register status bits
 */
#define	UURDB_ERROR	0x8000	/* Error (overrun or break) */
#define UURDB_ORUN	0x4000	/* Data overrun error */
#define	UURDB_BREAK	0x2000	/* TU58 break */

#define	UUDB_DMASK	0x00ff	/* data mask (send and receive data) */

