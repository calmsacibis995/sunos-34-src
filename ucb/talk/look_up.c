#ifndef lint
static	char sccsid[] = "@(#)look_up.c 1.1 86/09/25 SMI"; /* from UCB 1.2 83/03/28 */
#endif

#include "talk_ctl.h"

    /* see if the local daemon has a invitation for us */

CTL_RESPONSE swapresponse();

check_local()
{
    CTL_RESPONSE response;

	/* the rest of msg was set up in get_names */

    msg.ctl_addr = ctl_addr;

    if (!look_for_invite(&response)) {

	    /* we must be initiating a talk */

	return(0);
    }

        /*
	 * there was an invitation waiting for us, 
	 * so connect with the other (hopefully waiting) party 
	 */

    current_state = "Waiting to connect with caller";

    response = swapresponse(response);
    while (connect(sockt, &response.addr, sizeof(response.addr)) != 0) {
	if (errno == ECONNREFUSED) {

		/* the caller gave up, but his invitation somehow
		 * was not cleared. Clear it and initiate an 
		 * invitation. (We know there are no newer invitations,
		 * the talkd works LIFO.)
		 */

	    ctl_transact(his_machine_addr, msg, DELETE, &response);
	    close(sockt);
	    open_sockt();
	    return(0);
	} else if (errno == EINTR) {
		/* we have returned from an interupt handler */
	    continue;
	} else {
	    p_error("Unable to connect with initiator");
	}
    }

    return(1);
}

    /* look for an invitation on 'machine' */

look_for_invite(response)
CTL_RESPONSE *response;
{
    struct in_addr machine_addr;

    current_state = "Checking for invitation on caller's machine";

    ctl_transact(his_machine_addr, msg, LOOK_UP, response);

	/* the switch is for later options, such as multiple 
	   invitations */

    switch (response->answer) {

	case SUCCESS:

	    msg.id_num = response->id_num;
	    return(1);

	default :
		/* there wasn't an invitation waiting for us */
	    return(0);
    }
}

/*  
 * heuristic to detect if need to reshuffle CTL_RESPONSE structure
 */

#define swapshort(a) (((a << 8) | ((unsigned short) a >> 8)) & 0xffff)
#define swaplong(a) ((swapshort(a) << 16) | (swapshort(((unsigned)a >> 16))))

#ifdef sun
struct ctl_response_vax {
	char type;
	char answer;
	short junk;
	int id_num;
	struct sockaddr_in addr;
};

CTL_RESPONSE
swapresponse(rsp)
	CTL_RESPONSE rsp;
{
	struct ctl_response_vax swaprsp;
	
	if (rsp.addr.sin_family != AF_INET) {
		bcopy(&rsp, &swaprsp, sizeof(CTL_RESPONSE));
		swaprsp.addr.sin_family = swapshort(swaprsp.addr.sin_family);
		if (swaprsp.addr.sin_family == AF_INET) {
			rsp.addr = swaprsp.addr;
			rsp.type = swaprsp.type;
			rsp.answer = swaprsp.answer;
			rsp.id_num = swaplong(swaprsp.id_num);
		}
	}
	return rsp;
}
#endif

#ifdef vax
struct ctl_response_sun {
	char type;
	char answer;
	unsigned short id_num2;
	unsigned short id_num1;
	short sin_family;
	short sin_port;
	short sin_addr2;
	short sin_addr1;
};

CTL_RESPONSE
swapresponse(rsp)
	CTL_RESPONSE rsp;
{
	struct ctl_response_sun swaprsp;
	
	if (rsp.addr.sin_family != AF_INET) {
		bcopy(&rsp, &swaprsp, sizeof(struct ctl_response_sun));
		if (swaprsp.sin_family == swapshort(AF_INET)) {
			rsp.type = swaprsp.type;
			rsp.answer = swaprsp.answer;
			rsp.id_num = swapshort(swaprsp.id_num1)
			    | (swapshort(swaprsp.id_num2) << 16);
			rsp.addr.sin_family = swapshort(swaprsp.sin_family);
			rsp.addr.sin_port = swaprsp.sin_port;
			rsp.addr.sin_addr.s_addr =
			    swaprsp.sin_addr2 | (swaprsp.sin_addr1 << 16);
		}
	}
	return rsp;
}
#endif
