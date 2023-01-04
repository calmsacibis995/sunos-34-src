static char	sccsid[] = "@(#)mem.diag.c 1.1 9/25/86 Copyright Sun Microsystems";

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>


#define NMENU		(sizeof(memd_menu)/sizeof(memd_menu[0]))
#define exit		(*romp->v_exit_to_mon)

int	fill(), memdisp(), const(), checker(), mrandom(), maddress(),
	micronpat(), diagpat(), unique(), m_option(), p_option(), w_option(),
	s_option(), loop_seq(), e_option(), mem_help(), set_addr(), set_size(),
	disperrlog(), defaulttest(), leavegame();

struct menu memd_menu[]  = {
    {'A', "set default address  ", set_addr, "A address"},
    {'S', "set default size     ", set_size, "S size"},

    {'f', "fill memory          ", fill, "f address size pattern"},
    {'d', "display memory       ", memdisp, "d address size"},

    {'t', "default test         ", defaulttest, "t"},
    {'c', "constant pattern test", const, "c address size pattern passcnt"},
    {'r', "random pattern test  ", mrandom, "r address size seed passcnt"},
    {'a', "address test         ", maddress, "a address size passcnt"},
    {'u', "uniqueness test      ", unique, "u address size increment passcnt"},
    {'x', "checker pattern test ", checker, "x address size pattern passcnt"},
    {'g', "diagonal pattern test", diagpat, "g address size passcnt"},
    {'G', "micron pattern test  ", micronpat, "G address size passcnt"},

    {'m', "byte, word, long mode", m_option, "m [0|1|2]"},
    {'p', "parity check         ", p_option, "p [0|1]"},
    {'w', "wait on error        ", w_option, "w [0|1]"},
    {'s', "scopeloop on error   ", s_option, "s [0|1]"},
    {'e', "error message mode   ", e_option, "e [0|1]"},

    {'E', "display error log    ", disperrlog, "E"},

    {'l', "loop                 ", loop_seq, "l loopcount"},
    {'h', "help                 ", mem_help, "h"},
    {'q', "quit                 ", leavegame, "q"},
};

char		tokenbuf[512];
extern char	end;

char defaultcmd[] = "m 0 ; c . . 0 ; c . . 55 ; c . . aa ; c . . ff ; a ; u . . 5 ; r ; m 1 ; c . . 0 ; c . . 5555 ; c . . aaaa ; c . . ffff ; a ; u . . 5 ; r ; m 2 ; c . . 0 ; c . . 55555555 ; c . . aaaaaaaa ; c . . ffffffff ; a ; u . . 5 ; r ; m 1 ; x 100000 ; l *";

extern int	datamode, paritymode, errmode, errmessmode, errlimit;
extern int	toterr;
int		buserrcount;

u_long		*memaddr, memsize;
u_long		loopflag = 0, loopcount = 0xffffffff;

main() {
	int			n, mbuserr();
	register		i,ret;
	u_long			j, t1, t2;

	ex_vector->e_buserr = mbuserr;

	memaddr = (u_long *)(((long)(&end + 0x1000) + PAGEMASK) & ~PAGEMASK);
	memsize = (u_long)(*romp->v_memorysize);

/*
	for (j = (u_long)memaddr; j < memsize; j += 0x800) {
		map(j, 0x800, j, PM_MEM);
	}
 */
	map(0, memsize, 0, PM_MEM);

	/* initialize globals */
	datamode = 1;			/* word mode for testing */
	paritymode = 3;			/* parity generation and checking */

	errmode = 0;			/* no wait  no scopeloop */
	errmessmode = 1;		/* print all errors */
	errlimit = 0xffffffff;		/* stop after error limit reached */

	toterr = 0;
	buserrcount = 0;

	for (;;){
nexttest:
		if (*token == NULL){
			printf("\n\nCommand : ");
			gets(tokenbuf);
			tokenparse(tokenbuf);
			while (*token == NULL) {
				printf("\014");
				printf("Sun-2 Memory Diagnostic REV 1.1 9/25/86\n\n");
				printf("Start of memory under test : 0x%x\n",
					memaddr);
				printf("Size  of memory under test : 0x%x\n\n",
					memsize - (u_long)memaddr);
				printf("\nCommand selections:\n\n");
				for(i = 0; i < NMENU; i++)
					printf("  %c - %s\n",
						memd_menu[i].t_char,
						memd_menu[i].t_name);
				printf("\n\nCommand : ");
				gets(tokenbuf);
				tokenparse(tokenbuf);
			}
		}
		for (i = 0; i < NMENU ; i++){
			if (memd_menu[i].t_char == **token){
				testname = memd_menu[i].t_name;
#ifdef TIME
				clocksync();
				t1 = timefrom(0);
#endif TIME
				switch(ret = (*memd_menu[i].t_call)( memaddr,
						memsize, token++)){
					case 0:		/* ok */
#ifdef TIME
						t2 = timefrom(t1);
						printf("%d msec elapsed\n",
							t2);
#endif TIME
						break;
					default:
						printf("%s failed %d\n",
							testname, ret);
					case 'q':	/* quit test */
						*token = NULL;
						break;
				}
				if (*token != NULL && **token == SEPARATOR)
					++token;
				goto nexttest;
			}
		}
		printf("no such test as (%c)\n", **token);
		*token = NULL;
	}
}

mem_help() {

	register i;

	printf("\n\n");
	for (i = 0; i < NMENU; i++)
		printf("%c    %s        %s\n", memd_menu[i].t_char,
			memd_menu[i].t_name, memd_menu[i].t_help);
	return(0);
}

loop_seq() {
	if (!loopflag) {
		loopcount = eattoken(0xffffffff, 0xffffffff, 0xffffffff, 10);
		loopflag = loopcount;
	}
	if (loopflag && --loopcount) {
		printf("\nLoop Sequence Pass %d\n", loopflag - loopcount);
		token = tokens;
		return(0);
	}
	else {
		printf("\nLoop Sequence Pass %d\n", loopflag - loopcount);
		loopflag = 0;
		*token = NULL;
		return(0);
	}
}

defaulttest() {

	tokenparse(defaultcmd);
	return(0);
}

leavegame() {

	printf("bye kids!!\n");
	exit();
}

