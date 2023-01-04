#ifndef lint
static char	sccsid[] = "@(#)vid.120.pat.c 1.1 9/25/86 Copyright Sun Microsystems";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * *********************************************************************
 * Program Name    : vid.120.pat (video patterns)
 * Source File     : tom:/usr/tom/vid.pat/vid.120.pat.c
 * Original Engr   : Gale Snow
 * Date            : 07/16/84
 * Function        : Program "vid.120.pat" displays eight different
 *								   patterns (inverted or not) on the monitor of Model 
 *							 		 120 Sun Workstation for testing purposes.  It will
 *                   also run on the Model 100 Sun Worstation but, the 
 *                 	 menus will not be centered correctly.
 * Revision(1) Engr: Tom Kraus
 * Revision(1) Date: 10/03/84
 * Revision(s)     : The program has been enhanced so that the patterns 
 *                   can be dispalyed continuously (continuous mode) or,
 *                   as before, they can be displayed on an individual
 *                   basis (single selection mode).
 * Useage          : vid.120.pat
 * Options         :
 * *********************************************************************
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#undef VOID
#include <curses.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/bw2var.h>
#include <pixrect/pixfont.h>
#include "pat_icons.h"

#define NBRPTNS 8       /* number of available patterns */
#define ESCAPE  '\033'  /* escape character */

struct pixrect *screen;
PIXFONT	*pf;
u_long background=PIX_CLR, foreground=PIX_SET, menu_op=PIX_SRC;
int	savesigm;


main()
{
	int	invert=0,	i=0, alldone();
	int delay=1; /* in seconds */ 
	char sel='0', lastsel='0';
	long nctyped=0;

	initscr();
	savetty();
	crmode();
	noecho();
	signal(SIGHUP,alldone);
	signal(SIGINT,alldone);

	/* open screen pixrect. */
	screen = (struct pixrect *)pr_open("/dev/fb");
	if (screen == NULL) {
		printf ("can't open frame buffer\n");
		exit(1);
	}
	pf = pf_open("/usr/lib/fonts/fixedwidthfonts/gallant.r.10");
	if (pf == NULL) {
		printf ("can't open font file\n");
		exit(1);
	}

  dpgmins();
 	sel=getchar();
  pf_text(screen, 836, 450, PIX_SRC, pf, &sel);
  sleep(1);

	while(sel != 'q') {
  	switch(sel) {
  	case 'c': /* continuous mode */
			dcmdins(); 
 	    sel=getchar();
      pf_text(screen, 632, 600, PIX_SRC, pf, &sel);
      sleep(1);

    	while( (sel != 'q') && (sel != ESCAPE) ) {
  			if (sel == 'b') {
    			nctyped=0;
    			/*
    			 * continue displaying the patterns until
    		   * the user hits any key on the keyboard      
  				 */
    			for(i=0; nctyped == 0; i=((i+1) % (NBRPTNS*2)) ) { 
  					switch(i) {
  					case  0:
        		  background=PIX_CLR; /* do NOT   */
  						foreground=PIX_SET; /* invert   */
      				menu_op=PIX_SRC;    /* patterns */
  						vp0();
  						break;
  					case  1:
  						vp1();
  						break;
  					case  2:
  						vp2();
  						break;
  					case  3:
  						vp3();
  						break;
  					case  4:
  						vp4();
  						break;
  					case  5:
  						vp5();
  						break;
  					case  6:
  						vp6();
  						break;
  					case  7:
  						vp7();
  						break;
  					case  8:
  						background=PIX_SET;       /*   do     */ 
  						foreground=PIX_CLR;       /* invert   */
  						menu_op=PIX_NOT(PIX_SRC); /* patterns */
  						vp0();
  						break;
  					case  9:
  						vp1();
  						break;
  					case 10:
  						vp2();
  						break;
  					case 11:
  						vp3();
  						break;
  					case 12:
  						vp4();
  						break;
  					case 13:
  						vp5();
  						break;
  					case 14:
  						vp6();
  						break;
  					case 15:
  						vp7();
  						break;
  					}
    				sleep(delay); /* show pattern for "delay" secs */
  					ioctl(0, FIONREAD, &nctyped); /* set "nctyped" equal to 
  					     the number of characters that the user typed */
    			}
  				/*
  				 * consume all characters that the user may have typed to 
  				 * interrupt continuous mode except character 'q' (to quit)
					 * and <esc> (to return to the Primary Option Menu) 
  				 */
  				for(i=0; (i < nctyped) && (sel != 'q') && (sel != ESCAPE);
							i++) {
						sel=getchar();
					}
					if ( (sel != 'q') && (sel != ESCAPE) ) { /* allow Continuous*/
						sel='0';                               /* Mode Menu to be */
					}                                        /* displayed again */
  			} else {
			    dcmdins(); 
 	        sel=getchar();
          pf_text(screen, 632, 600, PIX_SRC, pf, &sel);
          sleep(1);
				}
			}
  		break;
  	case 's': /* single step mode */
    	dssmdins();
    	sel=getchar();
      pf_text(screen, 967, 725, PIX_SRC, pf, &sel);
      sleep(1);

    	while( (sel != 'q') && (sel != ESCAPE) ) {
    		switch(sel) {
    		case 'i':
    			invert=!invert;
    			if (invert != 0) {
    				background=PIX_SET;
    				foreground=PIX_CLR;
    				menu_op=PIX_NOT(PIX_SRC);
    			}
    			else {
    				background=PIX_CLR;
    				foreground=PIX_SET;
    				menu_op=PIX_SRC;
    			}
    			sel=lastsel; /* invert existing pattern on screen */
   				break;
   			case '1':
   				vp0();
    		  lastsel=sel;
    			sel=getchar();
   				break;
   			case '2':
    			vp1();
    		  lastsel=sel;
    			sel=getchar();
   				break;
    		case '3':
    			vp2();
    		  lastsel=sel;
    			sel=getchar();
    			break;
    		case '4':
    			vp3();
    		  lastsel=sel;
    			sel=getchar();
    			break;
    		case '5':
    			vp4();
    		  lastsel=sel;
    			sel=getchar();
    			break;
    		case '6':
    			vp5();
    		  lastsel=sel;
    			sel=getchar();
    			break;
    		case '7':
    			vp6();
    		  lastsel=sel;
    			sel=getchar();
    			break;
    		case '8':
    			vp7();
    		  lastsel=sel;
    			sel=getchar();
    			break;
    		default:
    			dssmdins();
    		  lastsel=sel;
    			sel=getchar();
          pf_text(screen, 967, 725, PIX_SRC, pf, &sel);
          sleep(1);
    			break;
    		}
    	}
  		break;
  	default:
      dpgmins();
   	  sel=getchar();
      pf_text(screen, 836, 450, PIX_SRC, pf, &sel);
      sleep(1);
  		break;
  	}
	}

 	alldone();	
}

/*
 * Function "alldone" .
 */
alldone() {

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, PIX_CLR, 0, 0, 0);
	resetty();
	clear();
	endwin();
	exit(0);
}


/*
 * Function "dpgmins" (display program instructions) informs the user 
 * how to use the program:
 *   (1) c = continuous mode.
 *   (2) s = single step mode
 *   (3) q = quit
 */
dpgmins() 

{
	/* clear screen */
	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, PIX_CLR, 0, 0, 0);

  /* draw border around instructions */
	pr_vector(screen, 258, 325, 258, 575, PIX_SET, 1);
	pr_vector(screen, 258, 575, 898, 575, PIX_SET, 1);
	pr_vector(screen, 898, 575, 898, 325, PIX_SET, 1);
	pr_vector(screen, 898, 325, 258, 325, PIX_SET, 1);

	/* print title */
	pf_text(screen, 308, 375, PIX_SRC, pf, "VIDEO PATTERNS");
	pr_vector(screen, 308, 385, 476, 385, PIX_SET, 1);
	pf_text(screen, 620, 375, PIX_SRC, pf, "Primary Option Menu");
	pr_vector(screen, 620, 385, 848, 385, PIX_SET, 1);

	/* print user instructions for program */
	pf_text(screen, 308, 450, PIX_SRC, pf,
		"Please select one of the following options:");
	pf_text(screen, 453, 475, PIX_SRC, pf,
		"c - continuous mode");
	pf_text(screen, 453, 500, PIX_SRC, pf,
		"s - single step mode");
	pf_text(screen, 453, 525, PIX_SRC, pf,
		"q - quit");
}


/*
 * Function "dcmdins" (display continuous mode instructions) informs the
 * user how to use continuous mode:
 *   (1) b = begin continuous mode
 *   (2) r = restart the program.
 *   (3) q = quit
 */
dcmdins() 

{
	/* clear screen */
	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, PIX_CLR, 0, 0, 0);

  /* draw border around instructions */
	pr_vector(screen, 258, 225, 258, 650, PIX_SET, 1);
	pr_vector(screen, 258, 650, 898, 650, PIX_SET, 1);
	pr_vector(screen, 898, 650, 898, 225, PIX_SET, 1);
	pr_vector(screen, 898, 225, 258, 225, PIX_SET, 1);

	/* print title */
	pf_text(screen, 308, 275, PIX_SRC, pf, "VIDEO PATTERNS");
	pr_vector(screen, 308, 285, 476, 285, PIX_SET, 1);
	pf_text(screen, 608, 275, PIX_SRC, pf, "Continuous Mode Menu");
	pr_vector(screen, 608, 285, 848, 285, PIX_SET, 1);

	/* print user instructions for continuous mode */
	pf_text(screen, 500, 350, PIX_SRC, pf, "Instructions:");
	pr_vector(screen, 500, 360, 656, 360, PIX_SET, 1);
	pf_text(screen, 308, 400, PIX_SRC, pf,
		"To return to the Primary Option Menu:  Type");
	pf_text(screen, 308, 425, PIX_SRC, pf,
		"<esc>  now  or  from  any  pattern display.");
	pf_text(screen, 308, 475, PIX_SRC, pf,
		"To  quit:   Type a  'q'  now  or  from  any");
	pf_text(screen, 308, 500, PIX_SRC, pf,
		"pattern  display.");
	pf_text(screen, 308, 550, PIX_SRC, pf,
		"To  begin  continuos  mode:   Type a  'b'.");
	pf_text(screen, 536, 600, PIX_SRC, pf, "Select:");
}


/*
 * Function "dssmdins" (dispaly single step mode instructions) informs
 * the user how to use single step mode:
 *   (1)  1 = display pattern0
 *   (2)  2 = display pattern1
 *   (3)  3 = display pattern2
 *   (4)  4 = display pattern3
 *   (5)  5 = display pattern4
 *   (6)  6 = display pattern5
 *   (7)  7 = display pattern6
 *   (8)  8 = display pattern7
 *   (9)  i = invert patterns
 *   (10) r = restart the program
 *   (11) q = quit.
 */
dssmdins() {
	int i, x, y;

  /* clear screen */
	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, PIX_CLR, 0, 0, 0);
	
  /* draw border around menu */
	pr_vector(screen,   50,   25,   50,  825, PIX_SET, 1);
	pr_vector(screen,   50,  825, 1125,  825, PIX_SET, 1);
	pr_vector(screen, 1125,  825, 1125,   25, PIX_SET, 1);
	pr_vector(screen, 1125,   25,   50,   25, PIX_SET, 1);

	/* print title */
	pf_text(screen, 100, 75, PIX_SRC, pf, "VIDEO PATTERNS");
	pr_vector(screen, 100, 85, 268, 85, PIX_SET, 1);
	pf_text(screen, 823, 75, PIX_SRC, pf, "Single Step Mode Menu");
	pr_vector(screen, 823, 85, 1075, 85, PIX_SET, 1);

  /* display avilable patterns */
	pf_text(screen, 100, 150, PIX_SRC, pf,
		"1    -           grid");
	pr_rop(screen, 230, 120, 48, 48, menu_op, &vpicon0, 0, 0);
	pf_text(screen, 100, 225, PIX_SRC, pf,
		"2    -           vertical hatch");
	pr_rop(screen, 230, 195, 48, 48, menu_op, &vpicon1, 0, 0);
	pf_text(screen, 100, 300, PIX_SRC, pf,
		"3    -           targets");
	pr_rop(screen, 230, 270, 48, 48, menu_op, &vpicon2, 0, 0);
	pf_text(screen, 100, 375, PIX_SRC, pf,
		"4    -           diagonal lines");
	pr_rop(screen, 230, 345, 48, 48, menu_op, &vpicon3, 0, 0);
	pf_text(screen, 100, 450, PIX_SRC, pf,
		"5    -           black white & grey vertical");
	pf_text(screen, 100, 475, PIX_SRC, pf,
		"                 stripes");
	pr_rop(screen, 230, 420, 48, 48, menu_op, &vpicon4, 0, 0);
	pf_text(screen, 100, 525, PIX_SRC, pf,
		"6    -           fine hatch");
	pr_rop(screen, 230, 495, 48, 48, menu_op, &vpicon5, 0, 0);
	pf_text(screen, 100, 600, PIX_SRC, pf,
		"7    -           coarse hatch");
	pr_rop(screen, 230, 570, 48, 48, menu_op, &vpicon6, 0, 0);
	pf_text(screen, 100, 675, PIX_SRC, pf,
		"8    -           fine hatch with boxes");
	pr_rop(screen, 230, 645, 48, 48, menu_op, &vpicon7, 0, 0);

	/* draw a border around the instructions */
	pr_vector(screen,  725,  150,  725,  775, PIX_SET, 1);
	pr_vector(screen,  725,  775, 1075,  775, PIX_SET, 1);
	pr_vector(screen, 1075,  775, 1075,  150, PIX_SET, 1);
	pr_vector(screen, 1075,  150,  725,  150, PIX_SET, 1);
  
	/* display instructions for single step mode */
	pf_text(screen, 835, 200, PIX_SRC, pf, "Instructions:");
	pr_vector(screen, 835, 210, 991, 210, PIX_SET, 1);
	pf_text(screen, 775, 250, PIX_SRC, pf,
		"To  select  a  pattern:");
	pf_text(screen, 775, 275, PIX_SRC, pf,
		"Type its number  (1-8).");
	pf_text(screen, 775, 325, PIX_SRC, pf,
		"To invert the patterns:");
	pf_text(screen, 775, 350, PIX_SRC, pf,
		"Type  an  'i'.");
	pf_text(screen, 775, 400, PIX_SRC, pf,
		"To return to this menu:");
	pf_text(screen, 775, 425, PIX_SRC, pf,
		"Type an  'r'  from  any");
	pf_text(screen, 775, 450, PIX_SRC, pf,
		"pattern  display.");
	pf_text(screen, 775, 500, PIX_SRC, pf,
		"To  return   to   the");
	pf_text(screen, 775, 525, PIX_SRC, pf,
		"Primary  Option  Menu:");
	pf_text(screen, 775, 550, PIX_SRC, pf,
		"Type <esc> now or from");
	pf_text(screen, 775, 575, PIX_SRC, pf,
		"any  pattern  display.");
	pf_text(screen, 775, 625, PIX_SRC, pf,
		"To quit: Type a  'q'");
	pf_text(screen, 775, 650, PIX_SRC, pf,
		"now  or  from   any");
	pf_text(screen, 775, 675, PIX_SRC, pf,
		"pattern  display.");
	pf_text(screen, 871, 725, PIX_SRC, pf, "Select:");
}


/* 
 * Functin "vp0" dispalys a grid pattern.
 */
vp0() {
	int	i;

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	for (i = 0; i < BW2SIZEX; i += 64) {
		pr_rop(screen, i, 0, 1, BW2SIZEY, foreground, 0, 0, 0);
		pr_rop(screen, i+31, 0, 1, BW2SIZEY, foreground, 0, 0, 0);
		pr_rop(screen, i+63, 0, 1, BW2SIZEY, foreground, 0, 0, 0);
	}
	for (i = 0; i < BW2SIZEY; i += 64) {
		pr_rop(screen, 0, i, BW2SIZEX, 1, foreground, 0, 0, 0);
		pr_rop(screen, 0, i+31, BW2SIZEX, 1, foreground, 0, 0, 0);
		pr_rop(screen, 0, i+63, BW2SIZEX, 1, foreground, 0, 0, 0);
	}
}


/* 
 * Functin "vp1" dispalys vertical hatch pattern.
 */
vp1() {
	int	i;

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	for (i = 0; i < BW2SIZEX; i += 2) {
		pr_rop(screen, i, 0, 1, BW2SIZEY, foreground, 0, 0, 0);
	}
}


/* 
 * Functin "vp2" dispalys targets.
 */
vp2() {
	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	pr_rop(screen, 0, 0, 4, 4, foreground, 0, 0, 0);
	pr_rop(screen, 0, BW2SIZEY-4, 4, 4, foreground, 0, 0, 0);
	pr_rop(screen, BW2SIZEX-4, 0, 4, 4, foreground, 0, 0, 0);
	pr_rop(screen, BW2SIZEX-4, BW2SIZEY-4, 4, 4, foreground, 0, 0, 0);
	pr_rop(screen, BW2SIZEX/2-2, BW2SIZEY/2-2, 4, 4, foreground, 0, 0, 0);
}


/* 
 * Functin "vp3" dispalys diagonal lines.
 */
vp3() {
	int	y, dx, i;

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	y = (BW2SIZEY / 64) * 64;
	dx = BW2SIZEY - y;
	for (i = 0; i < BW2SIZEY; i += 64) {
		pr_vector(screen, 0, y-i, i+dx, BW2SIZEY, foreground, 1);
	}
	for (i = 0; i < BW2SIZEX; i += 64) {
		pr_vector(screen, i, 0, BW2SIZEY+i, BW2SIZEY, foreground, 1);
	}
}


/* 
 * Functin "vp4" dispalys grey white black vertical stripes. 
 */
vp4() {
	int i, j;

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	for (i = 0; i < BW2SIZEX; i += 128) {
		for (j = 0; j < 32; j += 2) {
		    pr_rop(screen, i+j, 0, 1, BW2SIZEY, foreground, 0, 0, 0);
		}
		pr_rop(screen, i+64, 0, 32, BW2SIZEY, foreground, 0, 0, 0);
	}
}


/* 
 * Functin "vp5" dispalys hatch #1.
 */
vp5() {
	int x, y, z;

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	for ( y = BW2SIZEY, x=0; y > 0; y -= 2, x += 2 ) {
		pr_vector(screen, 0, y, x, BW2SIZEY, foreground, 1);
	}
	for ( z = 0; z < BW2SIZEX; z +=2, x += 2 ) {
		pr_vector(screen, z, 0, x, BW2SIZEY, foreground, 1);
	}
}



/* 
 * Functin "vp6" dispalys hatch #2.
 */
vp6() {
	int x, y, z;

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	for ( y = BW2SIZEY, x=0; x < BW2SIZEX + BW2SIZEY; y -= 4, x += 4 ) {
		pr_vector(screen, 0, y, x, BW2SIZEY, foreground, 1);
	}
	for ( x = 1, y = 1; x < BW2SIZEX + BW2SIZEY; y += 4, x += 4 ) {
		pr_vector(screen, x, 0, 0, y, foreground, 1);
	}
}


/* 
 * Functin "vp7" dispalys hatch #1 with black and white boxes.
 */
vp7() {
	int x, y, z;

	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, background, 0, 0, 0);
	/* fill in background grey hatching */
	for ( y = BW2SIZEY, x=0; y > 0; y -= 2, x += 2 ) {
		pr_vector(screen, 0, y, x, BW2SIZEY, foreground, 1);
	}
	for ( z = 0; z < BW2SIZEX; z +=2, x += 2 ) {
		pr_vector(screen, z, 0, x, BW2SIZEY, foreground, 1);
	}

	/* now throw up some black and white boxes */
	z = 0;	/* b/w flag */
	for ( y = 200; y <= 600; y += 200 ) {
		z = ~z;
		for ( x = 226; x <= 826; x += 200 ) {
			if (z)
			    pr_rop(screen, x, y, 100, 100, background, 0, 0, 0);
			else
			    pr_rop(screen, x, y, 100, 100, foreground, 0, 0, 0);
			z = ~z;
		}
	}
}

