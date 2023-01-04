
#ifndef lint
static	char sccsid[] = "@(#)error_checker.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
#include "install.h"


static	Boolean	all_ok;		/* Is everything ok to install? */

/*
 * Check to see if everything is in order to begin an installation.
 */
Boolean
everything_ok(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	int		tape_ix;
	char		*tape_string;

	all_ok = true;
	if (!tapedev_ok(ws, serv_arch)) {
		tape_ix = (int) setup_get(ws, WS_TAPE_TYPE);
		tape_string = setup_get(ws, SETUP_CHOICE_STRING, 
		    CONFIG_TAPETYPE, tape_ix);
		checker_msg(ws, SETUP_EBADTAPEDEV, tape_string);
	}
	if (isserver(ws)) {
		check_server(ws);
	} else if (isstandalone(ws)) {
		check_standalone(ws);
	} else {
		checker_msg(ws, SETUP_EWS_NONE);
	}
	return(all_ok);
}

/*
 * Check if all the necessary information has been supplied to install
 * a server.
 */
static
check_server(ws)
Workstation	ws;
{
	check_name(ws);
	check_hostnum(ws);
	check_ethernet_board(ws);
	check_yellow_pages(ws);
	check_tape(ws);
	check_all_clients(ws);
}

/*
 * A name must be supplied for the workstation.
 */
static
check_name(ws)
Workstation	ws;
{
	char		*name;

	name = setup_get(ws, WS_NAME);
	if (name == NULL || name[0] == '\0') {
		checker_msg(ws, SETUP_EWS_NONAME);
	}
}

/*
 * Check that the host number is legitimate.
 */
static
check_hostnum(ws)
Workstation	ws;
{
	char		*hostnum;
	int		etherindex;

	etherindex = (int) setup_get(ws, WS_ETHERTYPE);
	if (etherindex != 0) {
		hostnum = setup_get(ws, WS_HOST_NUMBER);
		if (hostnum[0] == '\0') {
			checker_msg(ws, SETUP_EWS_NOHOSTNUM);
		}
	}
}

/*
 * Check that an ethernet board has been selected.
 * This routine is called only for file servers which must have
 * ethernet boards.
 */
static
check_ethernet_board(ws)
Workstation	ws;
{
	int		etherindex;

	etherindex = (int) setup_get(ws, WS_ETHERTYPE);
	if (etherindex == 0) {
		checker_msg(ws, SETUP_EWS_NOETHERNET);
	}
}

/*
 * Check the yellow pages information.
 */
static
check_yellow_pages(ws)
Workstation	ws;
{
	Yp_type		yp_type;
	char		*master_name;
	char		*master_internet;
	char		*domain;

	yp_type = (Yp_type) setup_get(ws, WS_YPTYPE);
	if (yp_type == YP_SLAVE_SERVER) {
		master_name = setup_get(ws, WS_YPMASTER_NAME);
		if (master_name == NULL) {
			checker_msg(ws, SETUP_EWS_NOYPSERVER_NAME);
		}
		master_internet = setup_get(ws, WS_YPMASTER_INTERNET);
		if (master_internet == NULL) {
			checker_msg(ws, SETUP_EWS_NOYPSERVER_INTERNET);
		}
	}
	if (yp_type != YP_NONE) {
		domain = setup_get(ws, WS_DOMAIN);
		if (domain == NULL || domain[0] == '\0') {
			checker_msg(ws, SETUP_EWS_NODOMAIN);
		}
	}
}

/*
 * Check that all the tape information has been given.
 */
static
check_tape(ws)
Workstation	ws;
{
}

/*
 * Check that each client is fully specified.
 */
static
check_all_clients(ws)
Workstation	ws;
{
	Client		client;
	int		ix;

	SETUP_FOREACH_OBJECT(ws, WS_CLIENT, ix, client)	{
		check_client(ws, client);
	} SETUP_END_FOREACH
}

/*
 * Check an individual client.
 */
static
check_client(ws, client)
Workstation	ws;
Client		client;
{
	Soft_partition	root;
	Soft_partition	swap;
	char		*name;
	char		*eaddr;
	char		*hostnum;
	int		root_size;
	int		swap_size;

	name = setup_get(client, CLIENT_NAME);
	hostnum = setup_get(client, CLIENT_HOST_NUMBER);
	if (hostnum[0] == '\0') {
		checker_msg(ws, SETUP_ECLIENT_NOHOSTNUM, name);
	}
	eaddr = setup_get(client, CLIENT_E_ADDR);
	if (eaddr[0] == '\0') {
		checker_msg(ws, SETUP_ECLIENT_NOETHERNET, name);
	}

	root = (Soft_partition) setup_get(client, CLIENT_ROOT_PARTITION);
	if (root != NULL) {
		root_size = (int) setup_get(root, SOFT_SIZE);
		if (root_size == 0) {
			checker_msg(ws, SETUP_ECLIENT_NOROOTSIZE, name);
		}
	} else {
		checker_msg(ws, SETUP_ECLIENT_NOROOT, name);
	}

	swap = (Soft_partition) setup_get(client, CLIENT_SWAP_PARTITION);
	if (swap != NULL) {
		swap_size = (int) setup_get(swap, SOFT_SIZE);
		if (swap_size == 0) {
			checker_msg(ws, SETUP_ECLIENT_NOSWAPSIZE, name);
		}
	} else {
		checker_msg(ws, SETUP_ECLIENT_NOSWAP, name);
	}
}

/*
 * Check the info a standalone workstation
 */
static
check_standalone(ws)
Workstation	ws;
{
	check_name(ws);
	check_hostnum(ws);
	check_yellow_pages(ws);
	check_tape(ws);
}

/*
 * The checker has found an error.
 * Report it and set the flag.
 * If this is the first error, put out an initial message.
 */
static
checker_msg(ws, err, arg1, arg2, arg3)
Workstation	ws;
Setup_errno	err;
{
	if (all_ok == true) {
		runtime_message(SETUP_ECHECKBAD);
		message(ws, setup_msgbuf);
		all_ok = false;
	}
	runtime_message(err, arg1, arg2, arg3);
	message(ws, setup_msgbuf);
}
