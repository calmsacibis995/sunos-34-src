#ifndef lint
static	char sccsid[] = "@(#)in.talkd.c 1.1 86/09/25 SMI"; /* from UCB 1.5 83/05/10 */
#endif

/*
 * Invoked by the Internet daemon to handle talk requests
 * Processes talk requests until MAX_LIFE seconds go by with 
 * no action, then dies.
 */

#include "ctl.h"

#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>

extern	errno;

CTL_MSG request;
CTL_RESPONSE response;

char hostname[HOST_NAME_LENGTH];

int debug = 0;

CTL_MSG swapmsg();

main()
{
    struct sockaddr_in from;
    int from_size;
    int cc;
    int name_length = sizeof(hostname);
	int rfds;
	struct timeval tv;

    gethostname(hostname, &name_length);

    for (;;) {
	tv.tv_sec = MAX_LIFE;
	tv.tv_usec = 0;
	rfds = 1<<0;
	if (select(32, &rfds, 0, 0, &tv) <= 0)
		exit(0); 
	from_size = sizeof(from);
	cc = recvfrom(0, (char *)&request, sizeof (request), 0, 
		      &from, &from_size);

	if (cc != sizeof(request)) {
	    if (cc < 0 && errno != EINTR) {
		print_error("receive");
	    }
	} else {

	    if (debug) printf("Request received : \n");
	    if (debug) print_request(&request);

	    request = swapmsg(request);
	    process_request(&request, &response);

	    if (debug) printf("Response sent : \n");
	    if (debug) print_response(&response);

		/* can block here, is this what I want? */

	    cc = sendto(0, (char *) &response, sizeof(response), 0,
			&request.ctl_addr, sizeof(request.ctl_addr));

	    if (cc != sizeof(response)) {
		print_error("Send");
	    }
	}
    }
}

extern int sys_nerr;
extern char *sys_errlist[];

print_error(string)
char *string;
{
    FILE *cons;
    char *err_dev = "/dev/console";
    char *sys;
    int val, pid;

    if (debug) err_dev = "/dev/tty";

    sys = "Unknown error";

    if(errno < sys_nerr) {
	sys = sys_errlist[errno];
    }

	/* don't ever open tty's directly, let a child do it */
    if ((pid = fork()) == 0) {
	cons = fopen(err_dev, "a");
	if (cons != NULL) {
	    fprintf(cons, "Talkd : %s : %s(%d)\n\r", string, sys,
		    errno);
	    fclose(cons);
	}
	exit(0);
    } else {
	    /* wait for the child process to return */
	do {
	    val = wait(0);
	    if (val == -1) {
		if (errno == EINTR) {
		    continue;
		} else if (errno == ECHILD) {
		    break;
		}
	    }
	} while (val != pid);
    }
}

#define swapshort(a) (((a << 8) | ((unsigned short) a >> 8)) & 0xffff)
#define swaplong(a) ((swapshort(a) << 16) | (swapshort(((unsigned)a >> 16))))

/*  
 * heuristic to detect if need to swap bytes
 */

CTL_MSG
swapmsg(req)
	CTL_MSG req;
{
	CTL_MSG swapreq;
	
	if (req.ctl_addr.sin_family == swapshort(AF_INET)) {
		swapreq = req;
		swapreq.id_num = swaplong(req.id_num);
		swapreq.pid = swaplong(req.pid);
		swapreq.addr.sin_family = swapshort(req.addr.sin_family);
		swapreq.ctl_addr.sin_family =
			swapshort(req.ctl_addr.sin_family);
		return swapreq;
	}
	else
		return req;
}
