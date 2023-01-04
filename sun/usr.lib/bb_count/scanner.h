/*	@(#)scanner.h 1.1 86/09/25 SMI; from UCB X.X XX/XX/XX	*/

#include "defs.h"

#define EOS	'\0'
/*
 * Token values
 */
enum	token	{
	EOFT	 = 0,
	EMPTY	 = 1,
	LCURLY	 = '{',
	RCURLY	 = '}',
	LPAREN	 = '(',
	RPAREN	 = ')',
	LBRACKET = '[',
	RBRACKET = ']',
	SEMI	 = ';',
	COMMA	 = ',',
	EQUALS	 = '=',
	COLON	 = ':',
	ADD      = '+',
	SUB	 = '-',
	MUL	 = '*',
	DIV	 = '/',
	MOD	 = '%',
	GT	 = '>',
	LT	 = '<',
	NOT	 = '!',
	BITOR	 = '|',
	BITAND	 = '&',
	BITXOR	 = '^',
	STRING	 = 128,
	ID,
	NUM,
	AUTOINC,
	AUTODEC,
	LS,
	RS,
	LOGOR,
	LOGAND,
	GE,
	LE,
	EQ,
	NE,
	ASGADD,
	ASGSUB,
	ASGMUL,
	ASGDIV,
	ASGMOD,
	ASGLS,
	ASGRS,
	ASGOR,
	ASGAND,
	ASGXOR
};
