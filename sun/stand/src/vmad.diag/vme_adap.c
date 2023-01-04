/****************************************************************/
/* Main program for VME_multibus Adapter Diagnostic		*/
/****************************************************************/
static char     sccsid[] = "@(#)vme_adap.c 1.1 9/25/86 Copyright Sun Micro";

#include    <sunromvec.h>
#include    <sys/types.h>
#include    <setjmp.h>
#include    <machdep.h>
#include    <s2addrs.h>
#include    "vme_adap.h"



int  auto_diag(), manual_diag(), test_time_out(), test_data_path(),
         test_control_reg(), test_interrupt(), test_byte_mode(),
         test_word_mode(), run_all_tests(), debug_diag(), probe_tool(),
	 reset_tool(), interrupt_tool(), byte_dma_tool(), dma_tool(),
	 verify_reg_tool(), dummy();

static int menu_book[] = {5, 3, 7, 7};      /* max # of items in the menu */


struct menu test_menu[] = {
     0,   "Time Out ",          test_time_out,
     1,   "Data Path ",         test_data_path,
     2,   "Bus Arbit ",         test_control_reg,
     3,   "Interrupt ",         test_interrupt,
     4,   "Byte DMA ",          test_byte_mode,
     5,   "Word DMA ",          test_word_mode,
     6,   "All",                run_all_tests,
     -21, 0             /* starts on column 21 on screen */
};

struct menu mode[] = {
        0,   "Automatic",    auto_diag,
        1,   "Manual   ",    manual_diag,
	2,   "Debug    ",    debug_diag,
        -2,  0,         /* starts on column 2  on screen */
};


struct menu opr_keys[] = {
        0,   "^C abort test  ",    dummy,
        1,   "^S stop  loop  ",    dummy,
        2,   "^Q continue    ",    dummy,
        3,   "^X quit program",    dummy,
	4,   "^F on/off help ",    dummy,
        -61, 0,
};


struct menu debug_menu[] = {
	0,   "Probe Board",	probe_tool,
	1,   "Reset Board",	reset_tool,
	2,   "Interrupt  ",	interrupt_tool,
	3,   "DMA Write  ",	dma_tool,
	4,   "DMA Read   ",     dma_tool,
	5,   "Verify Data Reg", verify_reg_tool,
	6,   "Verify Count Reg",verify_reg_tool,
	-21, 0             /* starts on column 21 on screen */
};




extern	jmp_buf    get_out, intr_buf, bus_err;  /* declare none local jump */ 
extern 	struct   vme_adap_par       vap;
extern 	test_flag, debug_flag, dumterm;		 /* flag for menu printing  */

/****************************** BEGIN OF MAIN *********************************/
main()
{

   mymap(VME_ADAP_BASE, sizeof (struct prog_dma_reg), VME_PHYS_ADRS, PM_BUSMEM);

   bzero((u_short *)&vap, sizeof vap);
 
   vap.pdr = (struct prog_dma_reg*)VME_ADAP_BASE;
   save_sys_buserr_addr();		/*save system's bus err routine addr*/
   intr_init();
   dumterm = 0;

  /*
   * find out the type of terminal is being used[SUN/dum terminal]
   */
   if(*romp->v_insource == 0x01)
	dumterm = 1;

  /*
   * print headings
   */
   prt_headings();
   vap.info = 1;
   toggle_info_msg();                   /* turn off info msg     */

  /*
   * Begin executing the diagnostic
   */
   if ( !_setjmp(get_out))
      	execute_diag();
 
   restore_sys_buserr_handler();
	
   cup(23, 1);
   printf("\n ** EXIT DIAGNOSTIC ...!!!");
}
/**************************  END OF MAIN  *************************************/ 

 
 
 
 
/******************************************************************/
/* This function calls the Automatic or manual mode according to  */
/* the user choice.                                               */
/******************************************************************/
execute_diag()
{

int row, i;

   test_flag = 0;                       /* enable menu printing  */
   debug_flag = 0;
   draw_line();
   prt_menu(0);				/* special kesys         */
   prt_menu(1);     			/* Mode Menu		 */

  /*
   * Run manual or automatic test as requested by user.
   */
   while(1){
	if((i = get_user_choice(1)) < 0)
		return;
   	(mode[i].pd_test_ptr)();
   }
}



/******************************************************************/
/*  auto_diag function sets loop_num to one, calls run_all_tests  */
/* function.  If run_all_tests  returns a value of  0, indicates  */
/* that all tests have run successfully. It will ask the user if  */
/* he/she want to rerun and the number of times. If  a  return    */
/* value is other then 0, indicates that something has gone wrong,*/
/* it will terminates the test and return to calling function.    */
/******************************************************************/
auto_diag()
{
int     i, e = 0;		/* e != 0 indicates the error */
char	c;

   exchg_buserr_handler();              /* set up to catch bus error */
   intr_init();                         /* initialize intr service routine */

   vap.loop_num = 1;
   vap.man = 0x0;
   clear_lines();
   cup(17, 2);
   printf("AUTO   mode:"); 


   do{

      if ((e = run_all_tests()) == 0 ){

	 clear_below(21, 1);
	 cup(21, 1);
         printf(" VME ADAPTER DIAGNOSTIC COMPLETED...NO ERROR\n");

         if (pgetr(" Do you wish to run Diagnostic again ?[Y/N]  ")){
	 	cup(23, 1);
         	vap.loop_num = pmgn(" Enter number of times to run   ") + 1;
		cup(18, 1);
		printf("                                    ");
		clear_below(21, 1);
		continue;
         }else
         	_longjmp(get_out); 	/* return to main program  */
      }else 
		if(e == 0x03){          /* case ^C by the operator */
                	clear_below(21, 1);
                	cup(21, 1);
                	printf(" test aborted by operator");
                	return;
        	}else
                	break;	  

   } while(e == 0 && --vap.loop_num);   
 
   chk_err(e);
}


/******************************************************************/
/* This function prints tests menu and asks the user to select   */
/* a test to run. If all option is selected, it calls the         */
/* run_all_test function which increment throught all of the tests*/
/* otherwise an individual test is called to be executed.         */
/******************************************************************/
manual_diag()
{
   int     e, num = 0;		/* e != 0 indicates the error */
   int	   flag = 1;		/* make sure to print info only 1 time */
   struct  menu  *mp;

   exchg_buserr_handler();              /* set up to catch bus error */
   intr_init();                         /* initialize intr service routine */

   vap.man = 0x01;
   clear_lines();
   if(!test_flag){
   	prt_menu(2);
	test_flag = 1;
	debug_flag = 0;
   }


   while (1){

	if((num = get_user_choice(2)) < 0)
		return;			/* pop up the menu         */
        mp = &test_menu[0] + num;    	/* pointer = selected test.*/
	vap.all = ( mp->name == "All" ) ? 1 : 0;

	if(vap.info)
                prt_info_msg();

	cup(17, 2);
   	printf("MANUAL mode:");
	cup(17, 14);
        printf("%s ", mp->name);
	printf("test");
        vap.test_num = mp->test_num;
		
       /*
	* Execute the selected test.
	*/
        if ((e = (*mp->pd_test_ptr)()) == 0)
		continue;

	if(e == 0x03){			/* case ^C by the operator */
		clear_below(21, 1);
		cup(21, 1);
		printf(" %s test aborted by operator", mp->name);
		continue;
	}else
		break;
   }

   chk_err(e);
}




/******************************************************************/
/*  debug_diag executes a series of debuf tools to help the user  */
/* to nail the problem of the board. It will run for ever unless  */
/* the user aborts it.						  */
/******************************************************************/
debug_diag()
{

   struct  menu  *mp;
   int 	e, num = 0;

   exchg_buserr_handler();               /* setup to not abort at bus err */
   tool_intr_init();

   clear_lines();
   if(!debug_flag){
        prt_menu(3);
        debug_flag = 1;
	test_flag = 0;
   }


   while (1){
	
	if((num = get_user_choice(3)) < 0)
                return;                  /* pop up the menu         */
	
	mp = &debug_menu[0] + num;       /* pointer = selected test.*/
	vap.test_num = num;

   	if(vap.info)
                prt_info_msg();

	cup(17, 2);
   	printf("DEBUG  mode:");
	cup(17, 14);
	printf("                                ");
	cup(17, 16);
        printf("%s ", mp->name);
        printf("loop");

       /*
        * Execute the selected debug loop.
        */
        if ((e = (*mp->pd_test_ptr)()) == 0)
                continue;
	
	if(e == 0x03){                  /* case ^C by the operator */
                clear_below(21, 1);
                cup(21, 1);
                printf(" %s loop aborted by operator", mp->name);
                continue;
        }else     
                break;

    }

}
/******************************************************************/
/*  run_all_tests function calls and executes all of the tests    */
/* listed on menu. If any of the calling test return with a value */
/* of -1 , indicates that an error has occurres, an error handler */
/* routine is called to print out error messages, and eventually  */
/* return to calling function.                                    */
/******************************************************************/
run_all_tests()
{
   struct  menu  *mp;
   int     col, e;			/* e != 0 indicates the error */

   vap.all = 1;

   cup(18, 1);
   printf("                                      "); 
   if(vap.man){
      cup(21, 1);
      vap.loop_num = (pmgn(" Enter number of times to run: "));

    }

   cup(17, 14);
   printf("                                ");
   printf("                                ");

   for ( vap.pass_num = 1; vap.pass_num <= vap.loop_num ; vap.pass_num++){

      for ( mp = &test_menu[0]; (mp+1)->name != 0 ; mp++){

        col = mp->test_num * 11;
	cup(17, col + 14);
	if(vap.pass_num < 2)
		printf("%s+", mp->name);
 
        vap.test_num = mp->test_num;
        if ((e = (*mp->pd_test_ptr)()) != 0)
            return(e);
 
       }  
	cup(18, 1);
	printf("   looping ..... %d/%d", vap.loop_num, vap.pass_num);
 
   }
   prt_passed_msg();
   return(0);
}




/******************************************************************/
/* This function checks the return value to determine if any error*/
/* has occurred. If so it calls error_report function, otherwise  */
/* quit or exit the diagnostics  depending  on the return  value  */
/* passed by the calling function.                                */
/******************************************************************/
chk_err(e)
   int  e;
{
   struct  menu   *mp;

   switch (e){
      case -1 :
   	         mp = &test_menu[0] + vap.test_num;
		 clear_below(21, 1);
		 cup(21, 1);
       	         printf(" **Error: test %s failed.", mp->name);
                 error_report();
                 break;

      default:
                 break;


   }
}

 


/*********************************************************/
/* Print menu                                           */
/*********************************************************/

prt_menu(menu_level)
int	menu_level;
{
struct	menu *pt, *tmp;
int  	row = 5, col, i = 0;
	switch(menu_level){
		case 1:
			pt = mode;
			cup(3, 1);
			printf("    Mode Menu:");
			break;
		case 2:
			pt = test_menu;
			cup(3, 21);
			printf("    Test Menu: ");
			break;
		case 3: pt = debug_menu;
			cup(3, 21);
			printf("    Debug Menu:");
			break;
		case 0:
			pt = opr_keys;
			cup(3, 61);
			printf("    Special keys:");
			break;
	}
	
	tmp = pt;
	tmp += menu_book[menu_level];
	col = tmp->test_num * (-1) + 1;
	i = menu_book[menu_level];
	while(i--){
		cup(row++, col);
		switch(menu_level){
			case 2:
			case 3:
				printf("                       ");
				cup(row -1, col);
				printf(" %d -- %s   ", pt->test_num, pt->name);
				if(menu_level == 2 && pt->test_num == 2)
					printf("\b\b\b\bration ");
				break;
			case 1:
				printf(" %d -- %s   ", pt->test_num, pt->name);
				break;
			case 0:
				printf("   %s", pt->name);
				break;
	
		}
		pt++;
	}
	cup(row++, col);
	printf("                       ");
	cup(row++, col);
        printf("                       ");

}



/********************************************************/
/* Get user's choice.					*/
/* get the number of the selection from user. If no     */
/* number entered, user has chosen to pop up the menu  */
/* one level. If anything other than digits entered,    */
/* an error message will be displayed and the user will */
/* be asked to enter the number again.                  */
/********************************************************/
get_user_choice(menu_level)
int 	menu_level;
{
struct menu 	*pt;
int	row, col, tst_num, sub, i = 0;
	
	switch(menu_level){
		case 1:
                        pt = mode;
                        break;
                case 2:
		case 3:
			pt = mode;
			sub = (menu_level == 2) ? 1 : 2;
			row = menu_book[menu_level - sub] + 5;
			pt += menu_book[menu_level - sub];
			col = pt->test_num * (-1) + 2;
			cup(row, col);
                	printf("                 ");
			if(menu_level == 2)
                        	pt = test_menu;
			else
				pt = debug_menu;
                        break;
        }
	row = menu_book[menu_level] + 5;
	pt += menu_book[menu_level];
	col = pt->test_num * (-1) + 2;
	while (1){
                cup(row, col);
                printf("                ");
                cup(row, col);
                tst_num = pmgn("which one? ");
		clear_below(21, 1);

		if(tst_num >= 0 && tst_num < menu_book[menu_level]){
			vap.test_num = tst_num;
			cup(row, col);
			printf("                 ");
                        break;
		}

		cup(21, 2);                    /* move to message area */
		switch(tst_num){
			case -1:
				printf("Not a number");
				break;
			case -9:
				/*
				 * user choose to pop up the 
				 * menu one level           
				 */
				cup(row, col);
				printf("               ");
				return(-1);
			case -8:
				/*
				 * user does not want run diag 
				 * anymore, while program asking
				 * him to give selection.
				 */
				 _longjmp(get_out);
			case -7:
				/*
				 * Toggle info msg on
				 */
				 toggle_info_msg();
				 cup(17, 19);
				 break;
			default:
				printf("Number out of range: %d",
					tst_num); 
                                break;
		}
        }
 
        return(tst_num);

}


/*************************************************************/
/* Draw a line across the screen to devide the screen for    */
/* section: a- menu section, b- message section.            */
/*************************************************************/

draw_line()
{
	cup(16, 1);
	printf("YOU ARE RUNNING::");
	cup(19, 55);
	printf("Help option is %s", vap.info? "on " : "off");
	cup(20, 1);
	printf("*******************************  ");
	printf("MESSAGE AREA");
	printf("  ********************************");
}


mymap(virt, size, phys, space)
u_long		virt, size, phys;
enum pm_type	space;
{
	pg_t	page;
	register struct pg_field	*pgp = &page.pg_field;
	register			i;

	pgp->pg_valid = 1;
	pgp->pg_permission = PMP_ALL;
	pgp->pg_space = space;

	phys = BTOP(phys);
	size = BTOP(size);

	do {
		pgp->pg_pagenum = phys++;
		setpgreg(virt + PTOB(i), page.pg_whole);
		i++;
	} while(i < size);
}





clear_lines()
{
	cup(17, 1); 
	printf("                                       "); 
	printf("                                       ");
	printf("\n                                       ");
}
