#include <ctype.h>
#ifndef TOKEN_BUF_SIZE
#define TOKEN_BUF_SIZE 256
#endif
#include "token.h"

char  	*tokens[TOKEN_BUF_SIZE] = {NULL};
char	**token = tokens;
char	*testname;

static char	sccsid[] = "@(#)token.c 1.1 9/25/86 Copyright Sun Micro";

int
tokenparse(p)
register char		*p;
{
	extern char	tokenbuf[];
	register	seentoken = 0, ntoken = 0, len = bf_copy(p,tokenbuf);

	*(token = tokens) = NULL;		/* reset token pointer */
	p = tokenbuf;				/* eat up private copy */

	for (; len-- ; p++){
		if (isspace(*p)){		/* if white space, skip */
			*p = seentoken = 0;	/* null, and look for token */
		} else if (!seentoken) {	/* if char & token not seen */
			tokens[ntoken++] = p;	/* put token in list */
			tokens[ntoken] = NULL;	/* null next token */
			seentoken = 1;		/* and note token seen */
		}
	}
	return(ntoken);
}

long
eattoken(null, def, forever, specifiedbase)
long	null, def, forever, specifiedbase;
{
	if (*token == NULL || **token == SEPARATOR){
		return(null);
	} else if (**token == DEFAULT){
		++token;
		return(def);
	} else if (**token == FOREVER){
		++token;
		return(forever);
	} else {
		return(ktoi(*token++, specifiedbase));
	}
}

int
gettoken(null, buf)
register char	*null,*buf;
{
	if (*token == NULL || **token == SEPARATOR) {
		return(bf_copy(null, buf));
	} else {
		return(bf_copy(*token++, buf));
	}
}
int
bf_copy(from, to)
register char	*from, *to;
{
	register count = 0;

	while(*to++ = *from++)
		++count;
	return(count);
}
