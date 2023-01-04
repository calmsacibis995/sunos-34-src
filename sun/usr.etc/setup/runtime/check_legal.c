#ifndef lint
static	char sccsid[] = "@(#)check_legal.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
#include <ctype.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


legal_client_name(name, oldname)
char	*name;
char	*oldname;
{
	char	*ptr;
	int	n;
	Client	client;

	if (streq(name, oldname) && (! streq(name, ""))) {
		return(FALSE);
	}

	if (legal_name(name)) {
		return(TRUE);
	}

        ptr = (char *)setup_get(workstation, WS_NAME);
        if (streq(name, ptr)) {
		runtime_message(SETUP_EDUPNAME, name);
		return(TRUE);
        }

        ptr = (char *)setup_get(workstation, WS_TAPE_SERVER);
        if (streq(name, ptr)) {
		runtime_message(SETUP_EDUPNAME, name);
		return(TRUE);
        }

        ptr = (char *)setup_get(workstation, WS_YPMASTER_NAME);
        if (streq(name, ptr)) {
		runtime_message(SETUP_EDUPNAME, name);
		return(TRUE);
        }
 
        SETUP_FOREACH_OBJECT(workstation, WS_CLIENT, n, client)
                ptr = (char *)setup_get(client, CLIENT_NAME);
		if (streq(name, ptr)) {
			runtime_message(SETUP_EDUPNAME, name);
			return(TRUE);
		}
        SETUP_END_FOREACH

	return(FALSE);

	
}

legal_name(name)
char	*name;
{
	char	*ptr;

	if (streq(name, "")) {
		runtime_message(SETUP_ENULLNAME, name);
		return(TRUE);
	}

	for (ptr = name; *ptr != '\0'; ptr++) {
		if ((! isprint(*ptr)) || isspace(*ptr)) {
			runtime_message(SETUP_EBADNAME, name);
			return(TRUE);
		}
	}
	return(FALSE);
}

legal_card_name(name, oldname)
char	*name;
char	*oldname;
{
	char	*ptr;
	int	n;
	Card	card;

	if (streq(name, oldname) && !streq(name, "")) {
		return(FALSE);
	}
	if (*name == '\0') {
		runtime_message(SETUP_ENULLNAME, ptr);
		return(TRUE);
	}

	for (ptr = name; *ptr != '\0'; ptr++) {
		if (! isprint(*ptr)) {
			runtime_message(SETUP_EBADNAME, ptr);
			return(TRUE);
		}
	}

        SETUP_FOREACH_OBJECT(workstation, WS_CARD, n, card)
                ptr = (char *)setup_get(card, CLIENT_NAME);
		if (streq(name, ptr)) {
			runtime_message(SETUP_EDUPCARDNAME, name);
			return(TRUE);
		}
        SETUP_END_FOREACH

	return(FALSE);
}



legal_ethernet_addr(addr, old_addr)
char	*addr;
char	*old_addr;
{
	int	n;
	Client	client;
	char	*ptr;

	if (streq(addr, old_addr) && (! streq(addr, ""))) {
		return(FALSE);
	}

	if (legal_ethernet_addr_format(addr)) {
		return(TRUE);
	}

        SETUP_FOREACH_OBJECT(workstation, WS_CLIENT, n, client)
                ptr = (char *)setup_get(client, CLIENT_E_ADDR);
		if (streq(addr, ptr)) {
			runtime_message(SETUP_EDUPEADDR,
			    addr, (char *)setup_get(client, CLIENT_NAME));
			return(TRUE);
		}
        SETUP_END_FOREACH

	return(FALSE);

}

legal_ethernet_addr_format(addr)
char	*addr;
{

	int i, o[6];

	if (strlen(addr) > 17) {
		runtime_message(SETUP_EBADEADDR, addr);
		return(TRUE);
	}

	i = sscanf(addr, "%x:%x:%x:%x:%x:%x", &o[0], &o[1], &o[2],
					      &o[3], &o[4], &o[5]);
	if (i != 6) {
		runtime_message(SETUP_EBADEADDR, addr);
		return(TRUE);
	}

	for (i = 0; i < 6; i++) {
		if (o[i] > 0xff) {
			runtime_message(SETUP_EBADEADDR, addr);
			return(TRUE);
		}
	}

	if (o[0] & 0x01) {
		runtime_message(SETUP_EBADEADDR, addr);
		return(TRUE);
	}

	return(FALSE);
}


Network_class
legal_network(network)
char	*network;
{
	u_long		netnum;
	struct in_addr	in;
	u_long		ipaddr;
	char		*ptr;
	int		dotcnt;

	if (*network == '\0') {
		runtime_message(SETUP_ENULLNETWORK);
		return(BAD_NETWORK);
	}		

	netnum = inet_network(network);
	in = inet_makeaddr(netnum, 0);
	ipaddr = *(u_long *)&in;

	if (inet_netof(in) != netnum) {

		dotcnt = 0;
		for (ptr = network; *ptr != '\0'; ptr++) {
			if (*ptr == '.') {
				dotcnt++;
			}
		}

		switch (dotcnt) {
		case 0:
                        runtime_message(SETUP_EBADNETWORK, 'A', 1, 127);
			break;
		case 1:
                        runtime_message(SETUP_EBADNETWORK, 'B', 128, 191);
			break;
		case 2:
		default:
                        runtime_message(SETUP_EBADNETWORK, 'C', 192, 255);
			break;
		}

		return(BAD_NETWORK);
	}

	if (IN_CLASSA(ipaddr)) {
                return(CLASS_A_NETWORK);
	} else if (IN_CLASSB(ipaddr)) {
                return(CLASS_B_NETWORK);
	} else if (IN_CLASSC(ipaddr)) {
                return(CLASS_C_NETWORK);
	}

}


#define MAXHOSTS	256
static int	host_numbers[MAXHOSTS];
static int	host_cnt;

char *
legal_host_number(ptr)
char		*ptr;
{
	int		first_host_number;
	int		max_host_number;
	int		autohost;
	int		host_number;
	int		i;
	int		h;
	Network_class	class;
	int		compare();

        if ((class = (Network_class) setup_get(workstation, WS_NETWORK_CLASS))
            == BAD_NETWORK) {
                runtime_message(SETUP_ENONETWORK);
                return(NULL);
        }

	autohost = (int)setup_get(workstation, PARAM_AUTOHOST);
	first_host_number = (int)setup_get(workstation, PARAM_FIRSTHOST);
        max_host_number = (1 << ord(class)) - 2;

	get_host_numbers();
	qsort(host_numbers, host_cnt, sizeof(host_numbers[0]), compare);

	/*
	 * if we get the NULL string then automatically assign a host number
	 */
	if (*ptr == '\0') {

		if (!autohost) {
			runtime_message(SETUP_ENOAUTOHOST);
			return(NULL);
		}
		if (host_cnt == 0) {
			sprintf(scratch_buf, "%d", first_host_number);
			return(strdup(scratch_buf));
		}
			
		for (h = first_host_number; h < max_host_number; h++) {
			for (i = 0; i < host_cnt; i++) {
				if (h == host_numbers[i]) {
					break;		/* Number in use */
				}
				if (h < host_numbers[i]) {
					sprintf(scratch_buf, "%d", h);
					return(strdup(scratch_buf));
				}
			}
			if (i == host_cnt) {
				sprintf(scratch_buf, "%d", h);
				return(strdup(scratch_buf));
			}
		}
		runtime_message(SETUP_ENOMOREHOSTS);
		return(NULL);

	} else {

		host_number = atoi(ptr);

		if ((host_number < MIN_HOST_NUMBER) ||
		    (host_number > max_host_number)) {
			runtime_message(SETUP_EHOSTOUTOFRANGE, 
			    class_to_letter(class),
			    first_host_number,
			    max_host_number);
			return(NULL);
		}

                for (i = 0; i < host_cnt; i++) {
			if (host_numbers[i] > host_number) {
				break;
			}
                        if (host_numbers[i] == host_number) {
				runtime_message(SETUP_EHOSTNUMBERUSED, 
				    host_number);
				return(NULL);
                        } 
		}

		sprintf(scratch_buf, "%d", host_number);
		return((char *)strdup(scratch_buf));

	}
}


compare(a, b)
int	*a;
int	*b;
{
	if (*a < *b) {
		return(-1);
	} else if (*a > *b) {
		return(1);
	} else {
		return(0);
	}
}


get_host_numbers()
{
	char	*ptr;
	int	n;
	Client	client;

	host_cnt = 0;

        ptr = (char *)setup_get(workstation, WS_HOST_NUMBER);
	if (*ptr != '\0') {
		host_numbers[host_cnt++] = atoi(ptr);
	}

	SETUP_FOREACH_OBJECT(workstation, WS_CLIENT, n, client)
		ptr = (char *)setup_get(client, CLIENT_HOST_NUMBER);
		if (*ptr != '\0') {
			host_numbers[host_cnt++] = atoi(ptr);
		}
	SETUP_END_FOREACH

}

class_to_letter(class)
Network_class	class;
{
	switch(class) {
	case CLASS_A_NETWORK:
		return('A');
		break;
	case CLASS_B_NETWORK:
		return('B');
		break;
	case CLASS_C_NETWORK:
		return('C');
		break;
	}
}
