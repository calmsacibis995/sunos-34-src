
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_send.c 1.1 86/09/25 SMI"; /* from UCB 6.8 3/17/86 */
#endif LIBC_SCCS and not lint

/*
 * Send query to name server and wait for reply.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/nameser.h>
#include <arpa/resolv.h>

extern int errno;

#define KEEPOPEN (RES_USEVC|RES_STAYOPEN)

res_send(buf, buflen, answer, anslen)
	char *buf;
	int buflen;
	char *answer;
	int anslen;
{
	register int n;
	int retry, v_circuit, resplen, ns;
	static int s = -1;
	int gotsomewhere = 0;
	u_short id, len;
	char *cp;
	int dsmask;
	struct timeval timeout;
	HEADER *hp = (HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;

#ifdef DEBUG
	if (_res.options & RES_DEBUG) {
		printf("res_send()\n");
		p_query(buf);
	}
#endif DEBUG
	if (!(_res.options & RES_INIT))
		if (res_init() == -1) {
			return(-1);
		}
	v_circuit = (_res.options & RES_USEVC) || buflen > PACKETSZ;
	id = hp->id;
	/*
	 * Send request, RETRY times, or until successful
	 */
	for (retry = _res.retry; --retry >= 0; ) {
	   for (ns = 0; ns < _res.nscount; ns++) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("Querying server (# %d) address = %s\n", ns+1,
			      inet_ntoa(_res.nsaddr_list[ns].sin_addr.s_addr)); 
#endif DEBUG
		if (v_circuit) {
			/*
			 * Use virtual circuit.
			 */
			if (s < 0) {
				s = socket(AF_INET, SOCK_STREAM, 0);
				if (s < 0) {
					if (_res.options & RES_DEBUG)
#ifdef DEBUG
					    perror("socket failed\n");
#endif DEBUG
					continue;
				}
				if (connect(s, &(_res.nsaddr_list[ns]), 
				   sizeof(struct sockaddr)) < 0) {
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
						perror("connect failed");
#endif DEBUG
					(void) close(s);
					s = -1;
					continue;
				}
			}
			/*
			 * Send length & message
			 */
			len = htons(buflen);
			if (write(s, &len, sizeof(len)) != sizeof(len) ||
				    write(s, buf, buflen) != buflen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("write failed");
#endif DEBUG
				(void) close(s);
				s = -1;
				continue;
			}
			/*
			 * Receive length & response
			 */
			cp = answer;
			len = sizeof(short);
			while (len > 0 && (n = read(s, cp, len)) > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("read failed");
#endif DEBUG
				(void) close(s);
				s = -1;
				continue;
			}
			cp = answer;
			resplen = len = ntohs(*(short *)cp);
			while (len > 0 && (n = read(s, cp, len)) > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("read failed");
#endif DEBUG
				(void) close(s);
				s = -1;
				continue;
			}
		} else {
			/*
			 * Use datagrams.
			 */
			if (s < 0)
				s = socket(AF_INET, SOCK_DGRAM, 0);
#if	BSD >= 43
			if (connect(s, &_res.nsaddr_list[ns],
			    sizeof(struct sockaddr)) < 0 ||
			    send(s, buf, buflen, 0) != buflen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG) 
					printf("connect/send errno = %d\n",
					    errno);
#endif DEBUG
				continue;
			}
#else BSD
			if (sendto(s, buf, buflen, 0, &_res.nsaddr_list[ns],
			    sizeof(struct sockaddr)) != buflen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG) 
					perror("sendto");
#endif DEBUG
				continue;
			}
#endif BSD
			/*
			 * Wait for reply 
			 */
			timeout.tv_sec = 
		(_res.retrans <<(_res.retry - retry)) / _res.nscount;
			timeout.tv_usec = 0;
			if (timeout.tv_sec <= 0) timeout.tv_sec = 1;
wait:
			dsmask = 1 << s;
			n = select(s+1, &dsmask, 0, 0, &timeout);
			if (n < 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("select");
#endif DEBUG
				continue;
			}
			if (n == 0) {
				/*
				 * timeout
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("timeout\n");
#endif DEBUG
				gotsomewhere = 1;
				continue;
			}
			if ((resplen = recv(s, answer, anslen, 0)) <= 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("recvfrom");
#endif DEBUG
				continue;
			}
			gotsomewhere = 1;
			if (id != anhp->id) {
				/*
				 * response from old query, ignore it
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG) {
					printf("old answer:\n");
					p_query(answer);
				}
#endif DEBUG
				goto wait;
			}
			if (!(_res.options & RES_IGNTC) && anhp->tc) {
				/*
				 * get rest of answer
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("truncated answer\n");
#endif DEBUG
				(void) close(s);
				s = -1;
				retry = _res.retry;
				v_circuit = 1;
				continue;
			}
		}
#ifdef DEBUG
		if (_res.options & RES_DEBUG) {
			printf("got answer:\n");
			p_query(answer);
		}
#endif DEBUG
		/*
		 * We are going to assume that the first server is preferred
		 * over the rest (i.e. it is on the local machine) and only
		 * keep that one open.
		 */
		if ((_res.options & KEEPOPEN) == KEEPOPEN && ns == 0) {
			return (resplen);
		} else {
			(void) close(s);
			s = -1;
			return (resplen);
		}
	   }
	}
	(void) close(s);
	s = -1;
	if (v_circuit == 0 && gotsomewhere == 0)
		errno = ECONNREFUSED;
	else
		errno = ETIMEDOUT;
	return (-1);
}
