/*
        @(#)ansi_pos.c 1.1 9/25/86 Copyright Sun Micro
 */
#define ESC 0x1b
int 	dumterm;
cup(x,y)
register int x, y;
{
    if(dumterm)
	printf("%ca%dR%dC", ESC, x, y);
    else
	printf("%c[%d;%dH",ESC,x,y);
}

ed(){
    if(dumterm)
	printf("%c+", ESC);
    else
	printf("%c[J",ESC);
}

clear_screen(){
	cup(1,1);
	ed();
}


clear_below(row,column)
register int row,column;
{
	cup(row,column);
	if(dumterm)
		printf("%cy", ESC);
	else
		ed();
}

underline(row,column,length)
register int row,column,length;
{
}
blink(row,column,length)
register int row,column,length;
{
}
no_blink(row,column,length)
register int row,column,length;
{
}
