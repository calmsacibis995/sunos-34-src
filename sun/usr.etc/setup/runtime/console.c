
#ifndef lint
static	char sccsid[] = "@(#)console.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <strings.h>

static FILE	*console_fp;

/*
 * get a console message and write it out as a setup message
 */
get_console_message()
{
	char		string[BUFSIZ];
	char		msg_buf[BUFSIZ];
        Voidfunc        message_proc;

	fgets(string, sizeof(string), console_fp);
	*(rindex(string, '\n')) = '\0';
	sprintf(msg_buf, "Console msg: %s", string);
        message_proc = (Voidfunc) setup_get(workstation, SETUP_MESSAGE_PROC);
	message_proc(msg_buf);
}


/*
 * if we have the console mapped then poll for any input
 */
struct timeval	timeout;
poll_console()
{
	int	console_fd;
	int	readfds;

	console_fd = (int)setup_get(workstation, WS_CONSOLE_FD);
	if (console_fd) {
		while(TRUE) {
			readfds = (1 << console_fd);
			select(32, &readfds, 0, 0, &timeout);
			if (readfds & (1 << console_fd)) {
				get_console_message();
			} else {
				return;
			}
		}
	}
}


/*
 * map the console to a tty
 */
grab_console()
{
	char	c;
	char	*line;
	int	i;
	static int	p;
	static int	t;

	/*
	 * code to find a free pty stolen from in.rlogind.c
	 */
        for (c = 'p'; c <= 's'; c++) {
                struct stat stb;
                line = "/dev/ptyXX";
                line[strlen("/dev/pty")] = c;
                line[strlen("/dev/ptyp")] = '0';
                if (stat(line, &stb) < 0)
                        break;
                for (i = 0; i < 16; i++) {
                        line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
                        p = open(line, 2);
                        if (p > 0) {
				goto gotpty;
			}
                }
        }
	runtime_error("Could not find a free \"pty\" for the console.");

gotpty:
	line[strlen("/dev/")] = 't';
	t = open(line, 2);
	if (t > 0) {
		if (ioctl(t, TIOCCONS, 0) == -1) {
			runtime_error("Ioctl error setting console.");
		}
	} else {
		runtime_error("Could not open %s.", line);
	}
	setup_set(workstation, 
		WS_CONSOLE_FD, p,
		0);
	console_fp = fdopen(p, "r");
	setbuf(console_fp, NULL);
}

