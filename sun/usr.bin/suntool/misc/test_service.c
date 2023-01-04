#ifndef lint
static	char sccsid[] = "@(#)test_service.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

#include <sys/types.h>
#include  <sunwindow/notify.h>

void	seln_init_service();

main()
{
	seln_init_service();
	notify_start();
}
