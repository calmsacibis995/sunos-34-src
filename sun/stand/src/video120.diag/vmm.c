static char	sccsid[] = "@(#)vmm.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <s2addrs.h>
#include <token.h>
#include "video120.h"

#define NBRVMM (sizeof(vmmenu)/sizeof(vmmenu[0]))

extern int cp();      /* in .../stand/src/video120.diag/cp.c      */
extern int errlog();  /* in .../stand/src/video120.diag/errlog.c  */
extern int gotomm();  /* in .../stand/src/video120.diag/gotomm.c  */
extern int help();    /* in .../stand/src/video120.diag/help.c    */
extern int loop();    /* in .../stand/src/video120.diag/loop.c    */
extern int pem();     /* in .../stand/src/video120.diag/pem.c     */
extern int prompt();  /* in .../stand/src/video120.diag/prompt.c  */
extern int quit();    /* in .../stand/src/video120.diag/quit.c    */
extern int soe();     /* in .../stand/src/video120.diag/soe.c     */
extern int vmaddrs(); /* in .../stand/src/video120.diag/vmaddrs.c */
extern int vmbowm();  /* in .../stand/src/video120.diag/vmbowm.c  */
extern int vmchekr(); /* in .../stand/src/video120.diag/vmchekr.c */
extern int vmconst(); /* in .../stand/src/video120.diag/vmconst.c */
extern int vmdef();   /* in .../stand/src/video120.diag/vmdef.c   */
extern int vmrandm(); /* in .../stand/src/video120.diag/vmrandm.c */
extern int vmuniq();  /* in .../stand/src/video120.diag/vmuniq.c  */
extern int woe();     /* in .../stand/src/video120.diag/woe.c     */

struct menu vmmenu[] = {
  {'a', "Address Test               b: 51 s,     w: 28 s    ",
        vmaddrs, "a [count]"           },
  {'c', "Constant Pattern Test      b: 26 s,     w: 15 s    ",
        vmconst, "c [pattern] [count]" },
  {'r', "Random Pattern Test        b: 37 s,     w: 21 s    ",
        vmrandm, "r [seed] [count]"    },
  {'u', "Uniqueness Test            b: 42 s,     w: 23 s    ", 
        vmuniq , "u [constant] [count]"},
  {'x', "Checker Pattern Test       b: 5 m 58 s, w: 2 m 52 s", 
        vmchekr, "x [pattern] [count]" },
  {'d', "Default Video Memory Test  42 s                    ",
        vmdef  , "d"                   },
  {'M', "Print Error Messages?                              ",
        pem    , "M [0 | 1]"           },
  {'B', "Byte Or Word Mode?                                 ",
        vmbowm , "B [0 | 1]"           },
  {'K', "Check Parity?                                      ",
        cp     , "K [0 | 1]"           },
  {'W', "Wait (Stop) On Error?                              ",
        woe    , "W [0 | 1]"           },
  {'S', "Scopeloop On Error?                                ",
        soe    , "S [0 | 1]"           },
  {'l', "Loop                                               ",
        loop   , "l [loop_count]"      },
  {'e', "Error Log Display                                  ",
        errlog , "e"                   },
  {'h', "Help                                               ",
        help   , "h"                   },
  {'p', "Return To The Main Menu                            ",
        gotomm , "p"                   },
  {'q', "Quit                                               ",
        quit   , "q"                   }
};
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/mm.c.
 */
extern struct  menu *mptr; /* required for function "help" */
extern int     nbr;        /* required for function "help" */
extern char    errmsg[81]; /* holds error messages         */
extern char    tokenbuf[]; /* holds command line           */

char   vdefcmd[]="B 0 ; c 0a ; B 1 ; c ;";

/*
 * Function "vmm" (video memory menu) prompts the user for one or more
 * "video memory menu" commands and interprets the command(s) prior to 
 * taking the appropriate action(s) (i.e. go back to the "main menu", 
 * execute a test, display the error log, etc.).
 */
vmm() 
{
  char   goodopn='\0';    /* good (legal) menu option input by user   */
  int    i=0;             /* loop control variable                    */

  mptr=vmmenu; /* set ptr to CURRENT menu                   */
  nbr=NBRVMM;  /* the number of options on the CURRENT menu */
  errmsg[0]='\0';
  for(;;) { /* exit this infinite loop via function "quit" */
    while( (*token != NULL) && (**token == SEPARATOR) ) {
      ++token;
    }
    while(*token == NULL) {
      if (goodopn == 'h') { /* Do not print menu because        */
        goodopn='\0';       /* function "help" just printed it. */
      } else {
        vtitle();
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
     * Determine which test the user selected.
     */
    goodopn='\0';
    for(i = 0; ((i < NBRVMM) && (*token != NULL)); i++) {
      if (vmmenu[i].t_char == **token){
        if ( (goodopn=vmmenu[i].t_char) == 'h') { /* print menu title */
          vtitle();
        }
        token++;
	switch( (*vmmenu[i].t_call)() ) {
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
	  *token = NULL;
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
      } else if (i == (NBRVMM - 1)) {
        strcpy(errmsg, "'"); /* build "illegal option" error message */
        strcat(errmsg, &(**token));
        strcat(errmsg, "'");
        strcat(errmsg, " is an illegal option.");
	*token=NULL;
      }
    } /* end of for loop which checks which test was selected */
  } /* end of infinite for loop */
} /* end of function "vmm" */


/*
 * Function "vtitle" (video memory menu title) prints the title of the 
 * Video Memory Menu.
 */
vtitle()
{

  printf("\014"); /* clear screen */
  printf("Sun-2/120 Video Board Diagnostic       REV 1.1 9/25/86       ");
  printf("Video Memory Menu\n");
}
