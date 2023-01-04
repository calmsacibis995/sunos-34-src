/*	@(#)screg.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#define	HOST_ADDR	0x00	/* 0x80 is right but Sysgen violates spec */
#define	WAIT_COUNT	250000

/* 
 * SCSI Sun host adapter control registers.
 */

struct	scsi_ha_reg {		/* host adapter (I/O space) registers */
	u_char	data;		/* data register */
	u_char	unused;
	u_char	cmd_stat;	/* command/status register */
	u_char	unused2;
	u_short	icr;		/* interface control register */
	u_short	unused3;
	u_long	dma_addr;	/* dma base address */
	u_short	dma_count;	/* dma count register */
	u_char	unused4;
	u_char	intvec;		/* interrupt vector for VMEbus versions */
};

/*
 * bits in the interface control register 
 */
#define	ICR_PARITY_ERROR	0x8000
#define	ICR_BUS_ERROR		0x4000
#define	ICR_ODD_LENGTH		0x2000
#define	ICR_INTERRUPT_REQUEST	0x1000
#define	ICR_REQUEST		0x0800
#define	ICR_MESSAGE		0x0400
#define	ICR_COMMAND_DATA	0x0200	/* command=1, data=0 */
#define	ICR_INPUT_OUTPUT	0x0100	/* input=1, output=0 */
#define	ICR_PARITY		0x0080
#define	ICR_BUSY		0x0040
/* Only the following bits may usefully be set by the CPU */
#define	ICR_SELECT		0x0020
#define	ICR_RESET		0x0010
#define	ICR_PARITY_ENABLE	0x0008
#define	ICR_WORD_MODE		0x0004
#define	ICR_DMA_ENABLE		0x0002
#define	ICR_INTERRUPT_ENABLE	0x0001

/*
 * Compound conditions of icr bits message, command/data and input/output.
 */
#define	ICR_COMMAND	(ICR_COMMAND_DATA)
#define	ICR_STATUS	(ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)
#define	ICR_MESSAGE_IN	(ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)
#define	ICR_BITS	(ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)
