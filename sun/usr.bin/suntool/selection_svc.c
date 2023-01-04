#ifndef lint
static  char sccsid[] = "@(#)selection_svc.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include  <sunwindow/notify.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

extern void	seln_init_service();

#ifdef STANDALONE
main(argc, argv)
#else
selection_svc_main(argc, argv)
#endif
    int		  argc;
    char	**argv;
{
    int		  debug = 0;

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'd')
	debug = 1;
    seln_init_service(debug);
    (void)notify_start();
    
	EXIT(0);
}
