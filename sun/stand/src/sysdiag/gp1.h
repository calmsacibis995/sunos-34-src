/*     @(#)gp1.h 1.1 86/09/25 SMI      */

#define	VME_GP1BASE 0x210000		/* VME bus address and size */
#define	VME_GP1SIZE 0x10000

#define GP1_BOARD_IDENT_REG 0		/* Microstore Interface Registers */
#define GP1_CONTROL_REG 1
#define GP1_STATUS_REG 1
#define GP1_UCODE_ADDR_REG 2
#define GP1_UCODE_DATA_REG 3

#define GP1_SHMEM_OFFSET 0x8000		/* Start of shared memory with respect
					   to gp1 base address */

#define GP1_CR_CLRIF 0x8000		/* Control Register Bit Fields */
#define GP1_CR_IENBLE 0x0300
#define GP1_CR_RESET 0x0040
#define GP1_CR_VP_CONTROL 0x0038
#define GP1_CR_VP_STRT0 0x0020
#define GP1_CR_VP_HLT 0x0010
#define GP1_CR_VP_CONT 0x0008
#define GP1_CR_PP_CONTROL 0x0007
#define GP1_CR_PP_STRT0 0x0004
#define GP1_CR_PP_HLT 0x0002
#define GP1_CR_PP_CONT 0x0001

#define GP1_CR_INT_NOCHANGE 0x0000	/* Values for GP1_CR_IENBLE field */
#define GP1_CR_INT_ENABLE 0x0100
#define GP1_CR_INT_DISABLE 0x0200
#define GP1_CR_INT_TOGGLE 0x0300

#define GP1_SR_IFLG 0x8000		/* Status Register Bit Fields */
#define GP1_SR_IEN 0x4000
#define GP1_SR_RESET 0x0400
#define GP1_SR_VP_STATE 0x0200
#define GP1_SR_PP_STATE 0x0100
#define GP1_SR_VP_STATUS 0x00F0
#define GP1_SR_PP_STATUS 0x000F
