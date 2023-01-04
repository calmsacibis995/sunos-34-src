static char	sccsid[] = "@(#)mm.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * *********************************************************************
 * Program Name    : video120.diag
 * Source File     : tom:/usr/tom/stand/src/video120.diag/mm.c
 * Original Engr   : Tom Kraus
 * Date            : 01/22/85
 * Function        : Program "video120.diag" (video board diagnostic)
 *                   is a stand alone diagnostic which determines 
 *                   whether or not the three functional sections of the
 *                   Sun-2/120 Video Board are working correctly.  The
 *                   three functional sections of the Sun-2/120 Video 
 *                   Board are the (1) control register, (2) video
 *                   memory and (3) serial communications controller.
 * Revision(1) Engr: 
 * Revision(1) Date: 
 * Revision(s)     : 
 * Useage          : video120.diag
 * Options         :
 * *********************************************************************
 */

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <s2addrs.h>
#include <token.h>
#include "video120.h"

#define NBRMM (sizeof(mmenu)/sizeof(mmenu[0]))

extern int cp();      /* in .../stand/src/video120.diag/cp.c      */
extern int crint();   /* in .../stand/src/video120.diag/crint.c   */
extern int crm();     /* in .../stand/src/video120.diag/crm.c     */
extern int errlog();  /* in .../stand/src/video120.diag/errlog.c  */
extern int help();    /* in .../stand/src/video120.diag/help.c    */
extern int loop();    /* in .../stand/src/video120.diag/loop.c    */
extern int mmall();   /* in .../stand/src/video120.diag/mmall.c   */
extern int mmdef();   /* in .../stand/src/video120.diag/mmdef.c   */
extern int pem();     /* in .../stand/src/video120.diag/pem.c     */
extern int quit();    /* in .../stand/src/video120.diag/quit.c    */
extern int sccm();    /* in .../stand/src/video120.diag/sccm.c    */
extern int vidint();  /* in .../stand/src/video120.diag/vidint.c  */
extern int vmm();     /* in .../stand/src/video120.diag/vmm.c     */

struct menu mmenu[] = {
  {'c', "Control Register Menu                           ", crm   , 
        "c"             },
  {'s', "Serial Communications Controller Menu           ", sccm  , 
        "s"             },
  {'v', "Video Memory Menu                               ", vmm   , 
        "v"             },
  {'d', "Default Test Sequence                   2 m  4 s", mmdef , 
        "d"             },
  {'a', "Execute All Tests                      14 m 20 s", mmall , 
        "a"             },
  {'M', "Print Error Messages?                           ", pem   , 
        "M [0 | 1]"     },
  {'K', "Check Parity?                                   ", cp    , 
        "K [0 | 1]"     },
  {'l', "Loop                                            ", loop  , 
        "l [loop_count]"},
  {'e', "Error Log Display                               ", errlog, 
        "e"             },
  {'h', "Help                                            ", help  , 
        "h"             },
  {'q', "Quit                                            ", quit  , 
        "q"             }
};

BYTNODE byteers[MAXERRS]; /* byte error log (see ./video120.h for typedef)   */
WRDNODE worders[MAXERRS]; /* word error log (see ./video120.h for typedef)   */
SCCNODE sccers[MAXERRS];  /* scc error log (see ./video120.h for typedef)    */
struct  menu *mptr=mmenu; /* ptr to CURRENT menu                             */
int     nbr=NBRMM;        /* number of options on CURRENT menu               */
char    tokenbuf[512];    /* holds command line                              */
long    lflag=0;          /* required for "loop" function in loop.c          */
long    lcount=0;         /* required for "loop" function in loop.c          */
char    errmsg[81];       /* holds error messages                            */
long    nbyters=0;        /* number of byte data errors                      */
long    nwrders=0;        /* number of word data errors                      */
long    nsccers=0;        /* number of serial com. controller data errors    */
long    paritee=1;        /* 0=no parity error check, 1=parity error check   */
long    prterrs=1;        /* 0=don't print errors, 1=print errors            */
long    bowmode=1;        /* 0=byte mode, 1=word mode                        */
long    testcrm=0;        /* 0=not testing memorability of control register,
                             1=testing memorability of control register      */
long    errmode=0;        /* 0=no stop and no scopeloop on error, 
                             1=stop on error, 2=stop and scopeloop on error  */
long    intrupt=0;        /* 0=didn't execute video interrupt routine,
                             1=executed video interrupt routine              */
int     (*oldvctr)();     /* pointer to current video interrupt routine      */
char    mdefcmd[]="c ; m ; s ; c ; i ; j ; p ; s ; s . ; p ; v ; B 0 ; c 0a ; B 1 ; c ; p ;"; /* main menu default command line               */
char    mallcmd[]="c ; m ; s ; c ; i ; j ; p ; s ; s * ; p ; v ; a ; c ; r ;   u ; x ; B 0 ; a ; c ; r ; u ; x ; p ;"; /* main menu command line for        */
                                        /* executing all tests               */


/*
 * Function "main" is the driver for the program.  It prompts the user for one
 * or more "main menu" commands and interprets the command(s) prior to taking 
 * the appropriate action(s) (i.e. go to a submenu, execute a test, display 
 * the error log, etc.).
 */
main() 
{
  char   goodopn='\0';         /* good (legal) menu option input by user   */
  int    i=0;                  /* loop control variable                    */
  int    xnbr=nbr;             /* save current value for later restoration */
  struct menu *xptr=mptr;      /* save current value for later restoration */
  ex_vector->e_int[4]=vidint;  /* defines a new video interrupt routine    */
  asm("	movw	#0x2300, sr"); /* lower CPU interrupt priority level so 
                                  that video board interrupt test works    */

  oldvctr=ex_vector->e_int[4];
  tokenbuf[0]='\0';
  errmsg[0]='\0';
  for(i=0; i < MAXERRS; i++) { /* initialize error logs */
    byteers[i].tstname[0]='\0';
    byteers[i].addr='\0';
    byteers[i].exp='\0';
    byteers[i].obs='\0';
    worders[i].tstname[0]='\0';
    worders[i].addr='\0';
    worders[i].exp='\0';
    worders[i].obs='\0';
    sccers[i].tstname[0]='\0';
    sccers[i].exp='\0';
    sccers[i].obs='\0';
  }
  for(;;) { /* exit this infinite loop via function "quit" */
    while( (*token != NULL) && (**token == SEPARATOR) ) {
      ++token;
    }
    while(*token == NULL) {
      if (goodopn == 'h') { /* Do not print menu because        */
        goodopn='\0';       /* function "help" just printed it. */
      } else {
        mtitle();
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
    for(i=0; ((i < NBRMM) && (*token != NULL)); i++) {
      if (mmenu[i].t_char == **token) {
        if ( (goodopn=mmenu[i].t_char) == 'h') { /* print menu title */
          mtitle();
        }
        token++;
        /* 
         * Call the function corresponding to the menu option selected.
         */
        switch( (*mmenu[i].t_call)() ) { 
        case 0: /* test worked ok */
          /*
           * Since more than one option can be entered on the command line at
           * once, the loop control variable must be reset so that any test on 
           * the menu can be executed.
           */
          i=(-1); 
          break;
        default: /* 
                  * Either there was a user input error, the user terminated
                  * the Error Log Display or a test failed.  In any case, stop
                  * parsing current command line.
                  */
          if (goodopn == 'e') { /* build "Error Log Display" message */
            strcpy(errmsg, "Error Log Display terminated by user.");
	  } else {
            strcpy(errmsg, "Test '"); /* build "test failed" error message */
            strcat(errmsg, &goodopn);
            strcat(errmsg, "'");
            strcat(errmsg, " failed.");
	  }
          *token=NULL;
          break;
        }
        mptr=xptr; /* restore old value */
        nbr=xnbr;  /* restore old value */
        while( (*token != NULL) && (**token == SEPARATOR) ) {
          ++token;
        }
      } else if (i == (NBRMM -1)) {
        strcpy(errmsg, "'"); /* build "illegal option" error message */
        strcat(errmsg, &(**token));
        strcat(errmsg, "'");
        strcat(errmsg, " is an illegal option.");
	*token=NULL;
      }
    } /* end of for loop which checks which test was selected */ 
  } /* end of infinite for loop */
} /* end of main */


/*
 * Function "mtitle" (main menu title) prints the title of the Main Menu.
 */
mtitle()
{

  printf("\014"); /* clear screen */
  printf("Sun-2/120 Video Board Diagnostic           REV 1.1 9/25/86           ");
  printf("Main Menu\n");
}





