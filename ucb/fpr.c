
#ifndef lint
static	char sccsid[] = "@(#)fpr.c 1.1 86/09/25 SMI"; /* from UCB */
#endif

#include <stdio.h>

#define BLANK ' '
#define TAB '\t'
#define NUL '\000'
#define FF '\f'
#define BS '\b'
#define CR '\r'
#define VTAB '\013'
#define EOL '\n'

#define TRUE 1
#define FALSE 0

#define MAXCOL 170
#define TABSIZE 8
#define INITWIDTH 8

typedef
  struct column
    {
      int count;
      int width;
      char *str;
    }
  COLUMN;

char cc;
char saved;
int length;
char *text;
int highcol;
COLUMN *line;
int maxpos;
int maxcol;

extern char *malloc();
extern char *calloc();
extern char *realloc();



main()
{
  register int ch;
  register char ateof;
  register int i;
  register int errorcount;


  init();
  errorcount = 0;
  ateof = FALSE;

  ch = getchar();
  if (ch == EOF)
    exit(0);

  if (ch == EOL)
    {
      cc = NUL;
      ungetc((int) EOL, stdin);
    }
  else if (ch == BLANK)
    cc = NUL;
  else if (ch == '1')
    cc = FF;
  else if (ch == '0')
    cc = EOL;
  else if (ch == '+')
    cc = CR;
  else
    {
      errorcount = 1;
      cc = NUL;
      ungetc(ch, stdin);
    }

  while ( ! ateof)
    {
      gettext();
      ch = getchar();
      if (ch == EOF)
	{
	  flush();
	  ateof = TRUE;
	}
      else if (ch == EOL)
	{
	  flush();
	  cc = NUL;
	  ungetc((int) EOL, stdin);
	}
      else if (ch == BLANK)
	{
	  flush();
	  cc = NUL;
	}
      else if (ch == '1')
	{
	  flush();
	  cc = FF;
	}
      else if (ch == '0')
	{
	  flush();
	  cc = EOL;
	}
      else if (ch == '+')
	{
	  for (i = 0; i < length; i++)
	    savech(i);
	}
      else
	{
	  errorcount++;
	  flush();
	  cc = NUL;
	  ungetc(ch, stdin);
	}
    }

  if (errorcount == 1)
    fprintf(stderr, "Illegal carriage control - 1 line.\n");
  else if (errorcount > 1)
    fprintf(stderr, "Illegal carriage control - %d lines.\n", errorcount);

  exit(0);
}



init()
{
  register COLUMN *cp;
  register COLUMN *cend;
  register char *sp;


  length = 0;
  maxpos = MAXCOL;
  sp = malloc((unsigned) maxpos);
  if (sp == NULL)
    nospace();
  text = sp;

  highcol = -1;
  maxcol = MAXCOL;
  line = (COLUMN *) calloc(maxcol, (unsigned) sizeof(COLUMN));
  if (line == NULL)
    nospace();
  cp = line;
  cend = line + (maxcol-1);
  while (cp <= cend)
    {
      cp->width = INITWIDTH;
      sp = calloc(INITWIDTH, (unsigned) sizeof(char));
      if (sp == NULL)
	nospace();
      cp->str = sp;
      cp++;
    }
}



gettext()
{
  register int i;
  register char ateol;
  register int ch;
  register int pos;


  i = 0;
  ateol = FALSE;

  while ( ! ateol)
    {
      ch = getchar();
      if (ch == EOL || ch == EOF)
	ateol = TRUE;
      else if (ch == TAB)
	{
	  pos = (1 + i/TABSIZE) * TABSIZE;
	  if (pos > maxpos)
	    {
	      maxpos = pos + 10;
	      text = realloc(text, (unsigned) maxpos);
	      if (text == NULL)
		nospace();
	    }
	  while (i < pos)
	    {
	      text[i] = BLANK;
	      i++;
	    }
	}
      else if (ch == BS)
	{
	  if (i > 0)
	    {
	      i--;
	      savech(i);
	    }
	}
      else if (ch == CR)
	{
	  while (i > 0)
	    {
	      i--;
	      savech(i);
	    }
	}
      else if (ch == FF || ch == VTAB)
	{
	  flush();
	  cc = ch;
	  i = 0;
	}
      else
	{
	  if (i >= maxpos)
	    {
	      maxpos = i + 10;
	      text = realloc(text, (unsigned) maxpos);
	      if (text == NULL)
		nospace();
	    }
	  text[i] = ch;
	  i++;
	}
    }

  length = i;
}



savech(col)
int col;
{
  register char ch;
  register int oldmax;
  register COLUMN *cp;
  register COLUMN *cend;
  register char *sp;
  register int newcount;


  ch = text[col];
  if (ch == BLANK)
    return;

  saved = TRUE;

  if (col >= highcol)
    highcol = col;

  if (col >= maxcol)
    {
      oldmax = maxcol;
      maxcol = col + 10;
      line = (COLUMN *) realloc(line, (unsigned) maxcol*sizeof(COLUMN));
      if (line == NULL)
	nospace();
      cp = line + oldmax;
      cend = line + (maxcol - 1);
      while (cp <= cend)
	{
	  cp->width = INITWIDTH;
	  cp->count = 0;
	  sp = calloc(INITWIDTH, (unsigned) sizeof(char));
	  if (sp == NULL)
	    nospace();
	  cp->str = sp;
	  cp++;
	}
    }

  cp = line + col;
  newcount = cp->count + 1;
  if (newcount > cp->width)
    {
      cp->width = newcount;
      sp = realloc(cp->str, (unsigned) newcount*sizeof(char));
      if (sp == NULL)
	nospace();
      cp->str = sp;
    }
  cp->count = newcount;
  cp->str[newcount-1] = ch;
}



flush()
{
  register int i;
  register int anchor;
  register int height;
  register int j;


  if (cc != NUL)
    putchar(cc);

  if ( ! saved)
    {
      i = length;
      while (i > 0 && text[i-1] == BLANK)
	i--;
      length == i;
      for (i = 0; i < length; i++)
	putchar(text[i]);
      putchar(EOL);
      return;
    }

  for (i =0; i < length; i++)
    savech(i);

  anchor = 0;
  while (anchor <= highcol)
    {
      height = line[anchor].count;
      if (height == 0)
	{
	  putchar(BLANK);
	  anchor++;
	}
      else if (height == 1)
	{
	  putchar( *(line[anchor].str) );
	  line[anchor].count = 0;
	  anchor++;
	}
      else
	{
	  i = anchor;
	  while (i < highcol && line[i+1].count > 1)
	    i++;
	  for (j = anchor; j <= i; j++)
	    {
	      height = line[j].count - 1;
	      putchar(line[j].str[height]);
	      line[j].count = height;
	    }
	  for (j = anchor; j <= i; j++)
	    putchar(BS);
	}
    }

  putchar(EOL);
  highcol = -1;
}



nospace()
{
  fputs("Storage limit exceeded.\n", stderr);
  exit(1);
}
