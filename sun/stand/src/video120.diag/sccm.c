static char	sccsid[] = "@(#)sccm.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <s2addrs.h>
#include <token.h>
#include <zsreg.h>
#include "video120.h"

#define NBRSCCM (sizeof(sccmenu)/sizeof(sccmenu[0]))

extern int cp();      /* in .../stand/src/video120.diag/cp.c      */
extern int errlog();  /* in .../stand/src/video120diag/errlog.c   */
extern int gotomm();  /* in .../stand/src/video120diag/gotomm.c   */
extern int help();    /* in .../stand/src/video120diag/help.c     */
extern int loop();    /* in .../stand/src/video120diag/loop.c     */
extern int pem();     /* in .../stand/src/video120.diag/pem.c     */
extern int prompt();  /* in .../stand/src/video120.diag/prompt.c  */
extern int quit();    /* in .../stand/src/video120diag/quit.c     */
extern int scc();     /* in .../stand/src/video120diag/scc.c      */
extern int sccdef();  /* in .../stand/src/video120diag/sccdef     */

struct menu sccmenu[] = {
  {'s', "Serial Com. Controller Test              ", scc   ,   
        "s [baudrate] [count]"},
  {'d', "Default Serial Com. Controller Test  37 s", sccdef,   
        "d"                   },
  {'M', "Print Error Messages?                    ", pem   , 
        "M [0 | 1]"           },
  {'K', "Check Parity?                            ", cp    ,
        "K [0 | 1]"           },
  {'l', "Loop                                     ", loop  ,
        "l [loop_count]"      },
  {'e', "Error Log Display                        ", errlog,
        "e"                   },
  {'h', "Help                                     ", help  ,
        "h"                   },
  {'p', "Return To The Main Menu                  ", gotomm, 
        "p"                   },
  {'q', "quit                                     ", quit  ,
        "q"                   }
};
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/mm.c.
 */
extern struct  menu *mptr; /* required for function "help" */
extern int     nbr;        /* required for function "help" */
extern char    errmsg[81]; /* holds error messages         */
extern char    tokenbuf[]; /* holds command line           */

char   sdefcmd[]="s 300 ; s 9600 ;";

/*
 * Function "sccm" (serial communication controller menu) prompts the user for
 * one or more "serial communication controller menu" commands and interprets
 * the command(s) prior to taking the appropriate action(s) (i.e. go back to 
 * the "main menu", execute a test, display the error log, etc.).
 */
sccm() 
{
  char   goodopn='\0';    /* good (legal) menu option input by user   */
  int    i=0;             /* loop control variable                    */

  mptr=sccmenu;  /* set ptr to CURRENT menu                   */
  nbr=NBRSCCM;   /* the number of options on the CURRENT menu */
  errmsg[0]='\0';
  for(;;) { /* exit this infinite loop via function "quit" */
    while( (*token != NULL) && (**token == SEPARATOR) ) {
      ++token;
    }
    while(*token == NULL) {
      if (goodopn == 'h') { /* Do not print menu because        */
        goodopn='\0';       /* function "help" just printed it. */
      } else {
        stitle();
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
    for(i=0; ((i < NBRSCCM) && (*token != NULL)); i++) {
      if (sccmenu[i].t_char == **token){
        if ( (goodopn=sccmenu[i].t_char) == 'h') { /* print menu title */
	  stitle();
        }
        token++;
        /* 
         * Call the function corresponding to the menu option selected.
         */
        switch( (*sccmenu[i].t_call)() ) {
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
      } else if (i == (NBRSCCM - 1)) {
        strcpy(errmsg, "'"); /* build "illegal option" error message */
        strcat(errmsg, &(**token));
        strcat(errmsg, "'");
        strcat(errmsg, " is an illegal option.");
	*token=NULL;
      }
    } /* end of for loop which checks which test was selected */
  } /* end of infinite for loop */
} /* end of function "sccm" */


/*
 * Function "stitle" (serial communication controller menu title) prints the
 * title of the Serial Communication Controller Menu.
 */
stitle()
{

  printf("\014"); /* clear screen */
  printf("Sun-2/120 Video Board Diagnostic  REV 1.1 9/25/86  ");
  printf("Serial Com. Controller Menu\n");
}
