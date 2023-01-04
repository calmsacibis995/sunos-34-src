/*	@(#)pcc_defines.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#define	PCCINT INT
#define	PCCPTR 020
#define	PCCFUN 040
# define ISPCCTYPE(x) (( (x)&BTMASK) <= ULONG )

#define OPVALREST(op,val,rest) \
	pcc_word( ((((rest)&0177777)<<16)|(((val)&0377)<<8)|((op)&0377)));

#define CHARSPERWORD 4

#define BAD_PCCOP -1
#define NAME_PCCOP 2
#define ICON_PCCOP 4
#define PLUS_PCCOP 6
#define PLUSEQ_PCCOP 7
#define MINUS_PCCOP 8
#define MINUSEQ_PCCOP 9
#define NEG_PCCOP 10
#define STAR_PCCOP 11
#define STAREQ_PCCOP 12
#define INDIRECT_PCCOP 13
#define BITAND_PCCOP 14
#define BITANDEQ_PCCOP 15
#define BITOR_PCCOP 17
#define BITOREQ_PCCOP 18
#define BITXOR_PCCOP 19
#define BITXOREQ_PCCOP 20
#define QUEST_PCCOP 21
#define COLON_PCCOP 22
#define ANDAND_PCCOP 23
#define OROR_PCCOP 24
#define GOTO_PCCOP 37
#define LISTOP_PCCOP 56
#define ASSIGN_PCCOP 58
#define COMOP_PCCOP 59
#define SLASH_PCCOP 60
#define SLASHEQ_PCCOP 61
#define MOD_PCCOP 62
#define MODEQ_PCCOP 63
#define LSHIFT_PCCOP 64
#define LSHIFTEQ_PCCOP 65
#define RSHIFT_PCCOP 66
#define RSHIFTEQ_PCCOP 67
#define CALL_PCCOP 70
#define CALL0_PCCOP 72

#define NOT_PCCOP 76
#define BITNOT_PCCOP 77
#define EQ_PCCOP 80
#define NE_PCCOP 81
#define LE_PCCOP 82
#define LT_PCCOP 83
#define GE_PCCOP 84
#define GT_PCCOP 85
#define REG_PCCOP 94
#define OREG_PCCOP 95
#define STASG_PCCOP 98
#define STARG_PCCOP 99
#define STCALL_PCCOP 100
#define CONV_PCCOP 104
#define FORCE_PCCOP 108
#define CBRANCH_PCCOP 109
#define PASS_PCCOP 200
#define STMT_PCCOP 201
#define SWITCH_PCCOP 202
#define LBRACKET_PCCOP 203
#define RBRACKET_PCCOP 204
#define EOF_PCCOP 205
#define ARIF_PCCOP 206
#define LABEL_PCCOP 207

#define ASG 1+
