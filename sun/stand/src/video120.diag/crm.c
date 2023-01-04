static char	sccsid[] = "@(#)crm.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <s2addrs.h>
#include <token.h>
#include "video120.h"

#define NBRCRM (sizeof(crmenu)/sizeof(crmenu[0]))

extern int cp();      /* in .../stand/src/video120.diag/cp.c      */
extern int crcpy();   /* in .../stand/src/video120.diag/crcpy.c   */
extern int crdef();   /* in .../stand/src/video120.diag/crdef     */
extern int crint();   /* in .../stand/src/video120.diag/crint.c   */
extern int crjmp();   /* in .../stand/src/video120.diag/crjmp.c   */
extern int crmem();   /* in .../stand/src/video120.diag/crmem.c   */
extern int crscr();   /* in .../stand/src/video120.diag/crscr.c   */
extern int errlog();  /* in .../stand/src/video120.diag/errlog.c  */
extern int gotomm();  /* in .../stand/src/video120.diag/gotomm.c  */
extern int help();    /* in .../stand/src/video120.diag/help.c    */
extern int loop();    /* in .../stand/src/video120.diag/loop.c    */
extern int pem();     /* in .../stand/src/video120.diag/pem.c     */
extern int prompt();  /* in .../stand/src/video120.diag/prompt.c  */
extern int quit();    /* in .../stand/src/video120.diag/quit.c    */
extern int soe();     /* in .../stand/src/video120.diag/soe.c     */
extern int woe();     /* in .../stand/src/video120.diag/woe.c     */

struct menu crmenu[] = {
  {'m', "Memory Test Of Control Register       8 s ", crmem ,
        "m [beg_val [end_val]] [inc] [count]"},
  {'c', "Copy Enable/Disable Test              17 s", crcpy ,
        "c [beg_val [end_val]] [inc] [count]"},
  {'M', "Print Error Messages?                     ", pem   , 
        "M [0 | 1]"                          },
  {'K', "Check Parity?                             ", cp    , 
        "K [0 | 1]"                          },
  {'W', "Wait (Stop) On Error?                     ", woe   , 
        "W [0 | 1]"                          },
  {'S', "Scopeloop On Error?                       ", soe   , 
        "S [0 | 1]"                          },
  {'s', "Screen (Display) Enable/Disable Test  12 s", crscr ,
        "s [count]"                          },
  {'i', "Interrupt Enable/Disable Test         5 s ", crint ,
        "i [count]"                          },
  {'j', "Jumper Test                           9 s ", crjmp ,
        "j [count]"                          },
  {'d', "Default Control Register Test         52 s", crdef , 
        "d"                                  },
  {'l', "Loop                                      ", loop  , 
        "l [loop_count]"                     },
  {'e', "Error Log Display                         ", errlog,      
        "e"                                  },
  {'h', "Help                                      ", help  , 
        "h"                                  },
  {'p', "Return To The Main Menu                   ", gotomm, 
        "p"                                  },
  {'q', "Quit                                      ", quit  , 
        "q"                                  }
};
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/mm.c.
 */
extern struct  menu *mptr; /* required for function "help" */
extern int     nbr;        /* required for function "help" */
extern char    errmsg[81]; /* holds error messages         */
extern char    tokenbuf[]; /* holds command line           */

char   cdefcmd[]="m ; s ; c ; i ; j ; p ;";

/*
 * Function "crm" (control register menu) prompts the user for one or more
 * "control register menu" commands and interprets the command(s) prior to 
 * taking the appropriate action(s) (i.e. go back to the "main menu", execute 
 * a test, display the error log, etc.).
 */
crm() 
{
  char   goodopn='\0'; /* good (legal) menu option input by user   */
  int    i=0;          /* loop control variable                    */

  mptr=crmenu; /* set ptr to CURRENT menu                   */
  nbr=NBRCRM;  /* the number of options on the CURRENT menu */
  errmsg[0]='\0';
  for(;;) { /* exit this infinite loop via function "quit" */
    while( (*token != NULL) && (**token == SEPARATOR) ) {
      ++token;
    }
    while(*token == NULL) {
      if (goodopn == 'h') { /* Do not print menu because        */
        goodopn='\0';       /* function "help" just printed it. */
      } else {
        ctitle();
        pmenu();
        if (errmsg[0] != '\0') { /* user error or a test failed */
          printf("\n%s\n\n", errmsg);
          errmsg[0]='\0';
	}
      }
      prompt();
      gets(tokenbuf);
      tokenparse(tokenbuf);
    }
    /*
     * Determine which option the user selected.
     */
    goodopn='\0';
    for(i=0; ((i < NBRCRM) && (*token != NULL)); i++) {
      if (crmenu[i].t_char == **token) {
        if ( (goodopn=crmenu[i].t_char) == 'h') { /* print menu title */
          ctitle();
        }
        token++;
        /* 
         * Call the function corresponding to the menu option selected.
         */
	switch( (*crmenu[i].t_call)() ) {
        case 0: /* test worked ok */
          /*
           * Since more than one option can be entered on the command line at 
           * once, the loop control variable must be reset so that any test on
           * the menu can be executed.
           */
          i=(-1);
          break;
        case 'p': /* return to Main Menu */
          return(0);
          break; 
	case 'q': /* quit parsing current command line */
	  *token=NULL;
	  break;
	default: /*
	          * Either there was a user input error or a test failed.  
                  * In either case, stop parsing current command line.
                  */
          strcpy(errmsg, "Test '"); /* build "test failed" error message */
          strcat(errmsg, &goodopn);
          strcat(errmsg, "'");
          strcat(errmsg, " failed.");
	  *token=NULL;
	  break;
	}
        while( (*token != NULL) && (**token == SEPARATOR) ) {
	  ++token;
	}
      } else if (i == (NBRCRM - 1)) {
        strcpy(errmsg, "'"); /* build "illegal option" error message */
        strcat(errmsg, &(**token));
        strcat(errmsg, "'");
        strcat(errmsg, " is an illegal option.");
	*token=NULL;
      }
    } /* end of for loop which checks which test was selected */
  } /* end of infinite for loop */
} /* end of function "crm" */


/*
 * Function "ctitle" (control register menu title) prints the title of the 
 * Control Register Menu.
 */
ctitle()
{

  printf("\014"); /* clear screen */
  printf("Sun-2/120 Video Board Diagnostic     REV 1.1 9/25/86     ");
  printf("Control Register Menu\n");
}

