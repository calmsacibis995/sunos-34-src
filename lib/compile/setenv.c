/* @(#)setenv.c 1.1 86/09/24 SMI */

static	short	setenv_made_new_vector= 0;
extern	char	*getenv();
extern	char	**environ;
extern	char	*malloc();

#define NULL 0

char *setenv(name, value)
	char *name, *value;
{	char *p= NULL, **q;
	int length= 0, vl;

	if ((p= getenv(name)) == NULL) {	/* Allocate new vector */
		for (q= environ; *q != NULL; q++, length++);
		q= (char **)malloc(sizeof(char *)*(length+2));
		bcopy((char *)environ, ((char *)q)+sizeof(char *), sizeof(char *)*(length+1));
		if (setenv_made_new_vector++)
			free(environ);
		environ= q;}
	else { /* Find old slot */
		p-= strlen(name)+1;
		for (q= environ; *q != p; q++);};
	length= strlen(name);
	vl= strlen(value);
	if (!p || (length+vl+1 > strlen(p)))
		*q= p= malloc(length+vl+2);
	(void)strcpy(p, name); p+= length;
	*p++= '=';
	(void)strcpy(p, value);
	return(value);
}
