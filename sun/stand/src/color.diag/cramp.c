
static char	sccsid[] = "@(#)cramp.c 1.1 9/25/86 Copyright SMI";

/*
	cramp.c	routine to display various shades of red, green, & blue
		upon screen.
*/

#include "colorbuf.h"

char red_ramp[256], green_ramp[256], blue_ramp[256];

make_red(){

    register int i;
    register char *r,*g,*b;
    int wrcmap();

    r=red_ramp;
    g=green_ramp;
    b=blue_ramp;
    for(i=0;i<256;i++){
	*r++=i;
	*g++=0;
	*b++=0;
    }
    Set_CFunc(GR_copy);
    write_cmap(wrcmap,red_ramp,green_ramp,blue_ramp,0);
    Set_Video_Cmap(0);
}

make_green(){

    register int i;
    register char *r,*g,*b;
    int wrcmap();

    r=red_ramp;
    g=green_ramp;
    b=blue_ramp;
    for(i=0;i<256;i++){
	*r++=0;
	*g++=i;
	*b++=0;
    }
    Set_CFunc(GR_copy);
    write_cmap(wrcmap,red_ramp,green_ramp,blue_ramp,0);
    Set_Video_Cmap(0);
}
make_blue(){

    register int i;
    register char *r,*g,*b;
    int wrcmap();

    r=red_ramp;
    g=green_ramp;
    b=blue_ramp;
    for(i=0;i<256;i++){
	*r++=0;
	*g++=0;
	*b++=i;
    }
    Set_CFunc(GR_copy);
    write_cmap(wrcmap,red_ramp,green_ramp,blue_ramp,0);
    Set_Video_Cmap(0);
}

display_red(){

    register int i,c;

    make_red();
    c=0;
    for(i=0;i<64;i++)
        vector(c++,0,1,512,0);
    for(i=0;i<256;i++)
        vector(c++,0,1,512,i);
    for(i=255;i>=0;i--)
        vector(c++,0,1,512,i);
    for(i=0;i<64;i++)
        vector(c++,0,1,512,0);
}

display_green(){

    register int i,c;

    make_green();
    c=0;
    for(i=0;i<64;i++)
        vector(c++,0,1,512,0);
    for(i=0;i<256;i++)
        vector(c++,0,1,512,i);
    for(i=255;i>=0;i--)
        vector(c++,0,1,512,i);
    for(i=0;i<64;i++)
        vector(c++,0,1,512,0);
}

display_blue(){

    register int i,c;

    make_blue();
    c=0;
    for(i=0;i<64;i++)
        vector(c++,0,1,512,0);
    for(i=0;i<256;i++)
        vector(c++,0,1,512,i);
    for(i=255;i>=0;i--)
        vector(c++,0,1,512,i);
    for(i=0;i<64;i++)
        vector(c++,0,1,512,0);
}
