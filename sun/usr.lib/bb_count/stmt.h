/*	@(#)stmt.h 1.1 86/09/25 SMI; from UCB X.X XX/XX/XX	*/

enum	control	{
	NONE,			/* Not a control structure */
	IF,
	DO,
	FOR,
	WHILE,
	SWITCH,
	ELSE,
	CASE,
	DEFAULT,
};

struct	control_words {
	char	*word;
	Control	value;
} control_words[] = {
	"if",		IF,
	"do",		DO,
	"for",		FOR,
	"while",	WHILE,
	"switch",	SWITCH,
	"else", 	ELSE,
	"case",		CASE,
	"default", 	DEFAULT,
	NULL,		NONE 
};
