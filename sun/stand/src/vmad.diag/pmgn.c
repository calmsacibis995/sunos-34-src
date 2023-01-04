
static char     sccsid[] = "@(#)pmgn.c 1.1 9/25/86 Copyright Sun Micro";

/*
 * Print character string and get a number. 
 * Return the number.
 * It will return  -1 	in case of bad number
 * 		   -7   if ^F is entered as the first input
 *                 -8   if ^X is entered as the first input
 *		   -9   if RETURN is entered as the first input
 */


#define NULL    '\0'
#define BS	'\10'			/* back space	*/
#define DEL	'\177'			/* delete	*/

int pmgn(s)
char 	*s;
{
	char 	c, buf[80];
	register num = 0;
	int 	flag = 0;
	int 	cnt, j = 0;


	while((c = *s) != NULL){
		
		putchar(c);
		s++;
	}
	cnt = 0;
	while((c = getchar()) != '\n' && c != '\r'
		&& c != 0x18 && c != 0x06){ 
		cnt++;
		if(c == ' ' && j == 0)
			continue;	/* skip blanks in the beginning */
		if(c == BS ){
			j = (j > 0)? --j : 0;
			putchar(' '); putchar('\b');
			cnt = (cnt > 0)? --cnt : 0;
			continue;
		}
		if(c == DEL){
			j = (j > 0)? --j : 0;
			cnt = (cnt)? --cnt : 0;
			putchar('\b'); putchar(' '); putchar('\b');
			continue;
		}

		buf[j] = c;
		j++;
	}
	while(buf[--j] == ' ')
		continue;		/* skip blanks at the end*/

	if(c == 0x18)			/* ^X will get you out of*/
		return(-8);             /* the diagnostic        */

	if(c == 0x06)
		return(-7);             /* toggle info messg bit */
	buf[++j] = '\n';
	j = 0;
	while(buf[j] != '\n' && buf[j] != '\r'){
		if(buf[j] >= '0' && buf[j] <= '9'){
			num = num * 10 + buf[j] - '0';
			flag = 1;
			j++;
		}else{
			flag = 0;
			break;
		}
	}
	if(flag)
		return(num);
	if(buf[0] = '\n' || buf[0] == '\r')
				     	/* just RETURN key  will */
		return(-9);             /* pop menu one level up */

	return(-1);			/* bad number was entered*/
}
