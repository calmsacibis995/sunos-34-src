#ifndef lint
static char sccsid[] = "@(#)ipc_msg.c 1.1 86/09/25 Sun Micro";	/* from S5R2 6.1 */
#endif
/*
 *	Inter-Process Communication Message Facility.
 */

#define MSGDEBUG	/* TEMPORARY - turn on special-case debugging */

#include	"../h/param.h"
#include	"../h/user.h"
#include	"../h/kernel.h"
#include	"../h/systm.h"
#include	"../h/map.h"
#include	"../h/signal.h"
#include	"../h/ipc.h"
#include	"../h/msg.h"
#include	"../h/seg.h"
#include	"../h/proc.h"
#include	"../h/buf.h"

/* HACK :: msgbuf structure renamed to avoid kernel compilation conflicts */
#define msgbuf ipcmsgbuf


/* Convert bytes to msg segments. */
#define	btoq(X)	((X + msginfo.msgssz - 1) / msginfo.msgssz)


struct msg		*msgfp;	/* ptr to head of free header list */

struct ipc_perm		*ipcget();
struct msqid_ds		*msgconv();


/*
 *	msgconv - Convert a user supplied message queue id into a ptr to a
 *		msqid_ds structure.
 */

struct msqid_ds *
msgconv(id)
register int	id;
{
	register struct msqid_ds	*qp;	/* ptr to associated q slot */

	if (id < 0) {
		u.u_error = EINVAL;
		return (NULL);
	}
	qp = &msgque[id % msginfo.msgmni];

	if ((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
	    id / msginfo.msgmni != qp->msg_perm.seq) {
		u.u_error = EINVAL;
		return (NULL);
	}
	return (qp);
}

/*
 *	msgctl - Msgctl system call.
 */

msgctl()
{
	register struct a {
		int		msgid,
				cmd;
		struct msqid_ds	*buf;
	}		*uap = (struct a *)u.u_ap;
	struct msqid_ds			ds;	/* queue work area */
	register struct msqid_ds	*qp;	/* ptr to associated q */

	if ((qp = msgconv(uap->msgid)) == NULL)
		return;

	u.u_rval1 = 0;
	switch (uap->cmd) {
	case IPC_RMID:
		if (u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
		    && !suser())
			break;
		while (qp->msg_first)
			msgfree(qp, (struct msg *) NULL, qp->msg_first);
		qp->msg_cbytes = 0;
		if (uap->msgid + msginfo.msgmni < 0)
			qp->msg_perm.seq = 0;
		else
			qp->msg_perm.seq++;
		if (qp->msg_perm.mode & MSG_RWAIT)
			wakeup((caddr_t)&qp->msg_qnum);
		if (qp->msg_perm.mode & MSG_WWAIT)
			wakeup((caddr_t)qp);
		qp->msg_perm.mode = 0;
		break;
	case IPC_SET:
		if (u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
		     && !suser())
			break;
		if (u.u_error = copyin((caddr_t) uap->buf, (caddr_t) &ds,
		    sizeof (ds))) {
			break;
		}
		if (ds.msg_qbytes > qp->msg_qbytes && !suser())
			break;
		qp->msg_perm.uid = ds.msg_perm.uid;
		qp->msg_perm.gid = ds.msg_perm.gid;
		qp->msg_perm.mode = (qp->msg_perm.mode & ~0777) |
		    (ds.msg_perm.mode & 0777);
		qp->msg_qbytes = ds.msg_qbytes;
		qp->msg_ctime = (time_t) time.tv_sec;
		break;
	case IPC_STAT:
		if (ipcaccess(&qp->msg_perm, MSG_R))
			break;
		if (u.u_error = copyout((caddr_t) qp, (caddr_t) uap->buf,
		    sizeof (*qp))) {
			break;
		}
		break;
	default:
		u.u_error = EINVAL;
		break;
	}
}

/*
 *	msgfree - Free up space and message header, relink pointers on q,
 *	and wakeup anyone waiting for resources.
 */

msgfree(qp, pmp, mp)
register struct msqid_ds	*qp;	/* ptr to q of mesg being freed */
register struct msg		*pmp;	/* ptr to mp's predecessor */
register struct msg		*mp;	/* ptr to msg being freed */
{
	/* Unlink message from the q. */
	if (pmp == NULL)
		qp->msg_first = mp->msg_next;
	else
		pmp->msg_next = mp->msg_next;
	if (mp->msg_next == NULL)
		qp->msg_last = pmp;
	qp->msg_qnum--;
	if (qp->msg_perm.mode & MSG_WWAIT) {
		qp->msg_perm.mode &= ~MSG_WWAIT;
		wakeup((caddr_t)qp);
	}

	/* Free up message text. */
	if (mp->msg_ts)
		rmfree(msgmap, (long)btoq(mp->msg_ts), (long)(mp->msg_spot+1));

	/* Free up header */
	mp->msg_next = msgfp;
	if (msgfp == NULL)
		wakeup((caddr_t)&msgfp);
	msgfp = mp;
}

/*
 *	msgget - Msgget system call.
 */

msgget()
{
	register struct a {
		key_t	key;
		int	msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int				s;	/* ipcget status return */

	if ((qp = (struct msqid_ds *) ipcget(uap->key, uap->msgflg,
	    (struct ipc_perm *)msgque, msginfo.msgmni, sizeof (*qp), &s))
	    == NULL) {
		return;
	}

	if (s) {
		/* This is a new queue.  Finish initialization. */
		qp->msg_first = qp->msg_last = NULL;
		qp->msg_qnum = 0;
		qp->msg_qbytes = msginfo.msgmnb;
		qp->msg_lspid = qp->msg_lrpid = 0;
		qp->msg_stime = qp->msg_rtime = (time_t) 0;
		qp->msg_ctime = (time_t) time.tv_sec;
	}
	u.u_rval1 = qp->msg_perm.seq * msginfo.msgmni + (qp - msgque);
}

/*
 *	msginit - Called by main(main.c) to initialize message queues.
 */

msginit()
{
	register int		i;	/* loop control */
	register struct msg	*mp;	/* ptr to msg begin linked */

	/* initialize message buffer map and free all segments */
	rminit(msgmap, (long)msginfo.msgseg, (long)1, "msgmap", msginfo.msgmap);

	/* link free message headers together */
	for (i = 0, mp = msgfp = msgh;++i < msginfo.msgtql;mp++)
		mp->msg_next = mp + 1;
}

/*
 *	msgrcv - Msgrcv system call.
 */

msgrcv()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		long		msgtyp;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msg		*mp,	/* ptr to msg on q */
					*pmp,	/* ptr to mp's predecessor */
					*smp,	/* ptr to best msg on q */
					*spmp;	/* ptr to smp's predecessor */
	register struct msqid_ds	*qp;	/* ptr to associated q */
	struct msqid_ds			*qp1;
	int				sz;	/* transfer byte count */

#ifdef notdef
	sysinfo.msg++;			/* bump message send/rcv count */
#endif notdef
	if ((qp = qp1 = msgconv(uap->msqid)) == NULL)
		return;

	if (ipcaccess(&qp->msg_perm, MSG_R))
		goto msgrcv_out;
	if (uap->msgsz < 0) {
		u.u_error = EINVAL;
		goto msgrcv_out;
	}
	smp = spmp = NULL;
	goto findmsg1;

findmsg:
	if ((qp1 = msgconv(uap->msqid)) == NULL) {
		u.u_error = EIDRM;
		return;
	}
#ifdef MSGDEBUG
	if (qp1 != qp) {
	(void) printf("msgrcv phase error1: qp:0x%x  qp1:0x%x\n", qp, qp1);
		u.u_error = EIDRM;
		return;
	}
#endif MSGDEBUG
findmsg1:
	pmp = NULL;
	mp = qp->msg_first;
	if (uap->msgtyp == 0)
		smp = mp;
	else
		for (;mp;pmp = mp, mp = mp->msg_next) {
			if (uap->msgtyp > 0) {
				if (uap->msgtyp != mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
				break;
			}
			if (mp->msg_type <= -uap->msgtyp) {
				if (smp && smp->msg_type <= mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
			}
		}
	if (smp) {
		if (uap->msgsz < (int) smp->msg_ts)
			if (!(uap->msgflg & MSG_NOERROR)) {
				u.u_error = E2BIG;
				goto msgrcv_out;
			} else
				sz = uap->msgsz;
		else
			sz = smp->msg_ts;
		if (u.u_error = copyout((caddr_t) &smp->msg_type,
		    (caddr_t) uap->msgp, sizeof (smp->msg_type))) {
			goto msgrcv_out;
		}
		if (sz) {
			if (u.u_error = copyout(
			    (caddr_t)(msg + msginfo.msgssz * smp->msg_spot),
			    ((caddr_t)uap->msgp + sizeof (smp->msg_type)),
			    (u_int)sz))
				goto msgrcv_out;
		}
		u.u_rval1 = sz;
		qp->msg_cbytes -= smp->msg_ts;
		qp->msg_lrpid = u.u_procp->p_pid;
		qp->msg_rtime = (time_t) time.tv_sec;
		curpri = PMSG;
		msgfree(qp, spmp, smp);
		goto msgrcv_out;
	}
	if (uap->msgflg & IPC_NOWAIT) {
		u.u_error = ENOMSG;
		goto msgrcv_out;
	}
	qp->msg_perm.mode |= MSG_RWAIT;

	if (sleep((caddr_t)&qp->msg_qnum, PMSG | PCATCH)) {
		u.u_error = EINTR;
		return;
	}
	goto findmsg;

msgrcv_out:
	;
}

/*
 *	msgsnd - Msgsnd system call.
 */

msgsnd()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	register struct msg		*mp;	/* ptr to allocated msg hdr */
	register int			cnt,	/* byte count */
					spot;	/* msg pool allocation spot */
	struct msqid_ds			*qp1;
	long				type;	/* msg type */

#ifdef notdef
	sysinfo.msg++;			/* bump message send/rcv count */
#endif notdef
	if ((qp = qp1 = msgconv(uap->msqid)) == NULL)
		return;

	if (ipcaccess(&qp->msg_perm, MSG_W))
		goto msgsnd_out;
	if ((cnt = uap->msgsz) < 0 || cnt > msginfo.msgmax) {
		u.u_error = EINVAL;
		goto msgsnd_out;
	}
	if (u.u_error = copyin((caddr_t) uap->msgp, (caddr_t) &type,
	    sizeof (type))) {
		goto msgsnd_out;
	}
	if (type < 1) {
		u.u_error = EINVAL;
		goto msgsnd_out;
	}
	goto getres1;		/* skip unlock/lock sequence */

getres:
	/* Be sure that q has not been removed. */

	if ((qp1 = msgconv(uap->msqid)) == NULL) {
		u.u_error = EIDRM;
		return;
	}
#ifdef MSGDEBUG
	if (qp1 != qp) {
	(void) printf("msgsnd phase error1: qp:0x%x  qp1:0x%x\n", qp, qp1);
		u.u_error = EIDRM;
		return;
	}
#endif MSGDEBUG
getres1:
	/* Allocate space on q, message header, & buffer space. */
	if (cnt + qp->msg_cbytes > qp->msg_qbytes) {
		if (uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			goto msgsnd_out;
		}
		qp->msg_perm.mode |= MSG_WWAIT;

		if (sleep((caddr_t)qp, PMSG | PCATCH)) {
			qp1 = msgconv(uap->msqid);
			u.u_error = EINTR;
			if (qp1 == NULL)
				return;
#ifdef MSGDEBUG
			if (qp1 != qp) {
	(void) printf("msgsnd phase error2: qp:0x%x  qp1:0x%x\n", qp, qp1);
				return;
			}
#endif MSGDEBUG
			qp->msg_perm.mode &= ~MSG_WWAIT;
			wakeup((caddr_t)qp);
			goto msgsnd_out;
		}
		goto getres;
	}
	if (msgfp == NULL) {
		if (uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			goto msgsnd_out;
		}

		if (sleep((caddr_t)&msgfp, PMSG | PCATCH)) {
			qp1 = msgconv(uap->msqid);
			u.u_error = EINTR;
			if (qp1 == NULL)
				return;
#ifdef MSGDEBUG
			if (qp1 != qp) {
	(void) printf("msgsnd phase error3: qp:0x%x  qp1:0x%x\n", qp, qp1);
				return;
			}
#endif MSGDEBUG
			goto msgsnd_out;
		}
		goto getres;
	}
	if (cnt && (spot = rmalloc(msgmap, (long)btoq(cnt))) == NULL) {
		if (uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			goto msgsnd_out;
		}
		mapwant(msgmap)++;

		if (sleep((caddr_t)msgmap, PMSG | PCATCH)) {
			qp1 = msgconv(uap->msqid);
			u.u_error = EINTR;
			if (qp1 == NULL)
				return;
#ifdef MSGDEBUG
			if (qp1 != qp) {
	(void) printf("msgsnd phase error4: qp:0x%x  qp1:0x%x\n", qp, qp1);
				return;
			}
#endif MSGDEBUG
			goto msgsnd_out;
		}
		goto getres;
	}

	/* Everything is available, copy in text and put msg on q. */
	mp = msgfp;
	msgfp = mp->msg_next;
	if (cnt) {
		if (u.u_error = copyin(((caddr_t)uap->msgp + sizeof (type)),
		    (caddr_t)(msg + msginfo.msgssz * --spot), (u_int)cnt)) {
			rmfree(msgmap, (long)btoq(cnt), (long)(spot + 1));
			goto msgsnd_out1;
		}
	}
	qp->msg_qnum++;
	qp->msg_cbytes += cnt;
	qp->msg_lspid = u.u_procp->p_pid;
	qp->msg_stime = (time_t) time.tv_sec;
	mp->msg_next = NULL;
	mp->msg_type = type;
	mp->msg_ts = cnt;
	mp->msg_spot = cnt ? spot : (ushort) -1;
	if (qp->msg_last == NULL)
		qp->msg_first = qp->msg_last = mp;
	else {
		qp->msg_last->msg_next = mp;
		qp->msg_last = mp;
	}
	if (qp->msg_perm.mode & MSG_RWAIT) {
		qp->msg_perm.mode &= ~MSG_RWAIT;
		curpri = PMSG;
		wakeup((caddr_t)&qp->msg_qnum);
	}
	u.u_rval1 = 0;
	goto msgsnd_out;

msgsnd_out1:

#ifdef MSGDEBUG
	if (mp->msg_next != msgfp)
	(void) printf("msgsnd phase error5: msgfp:0x%x  msg_next:0x%x\n",
				    msgfp, mp->msg_next);
#endif MSGDEBUG
	if (msgfp == NULL)
		wakeup((caddr_t)&msgfp);
	msgfp = mp;

msgsnd_out:
	;
}

/*
 *	msgsys - System entry point for msgctl, msgget, msgrcv, and msgsnd
 *		system calls.
 *		Assumes MSGGET / MSGCTL / MSGSND / MSGRCV definitions in
 *				...src/lib/libc/sys/msgsys.c
 */

msgsys()
{
	int		msgctl(),
			msgget(),
			msgrcv(),
			msgsnd();
	static int	(*calls[])() = { msgget, msgctl, msgrcv, msgsnd };
	register struct a {
		unsigned	id;	/* function code id */
		int		*ap;	/* arg pointer for recvmsg */
	}		*uap = (struct a *)u.u_ap;

	if (uap->id > 3) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*calls[uap->id])();
}
