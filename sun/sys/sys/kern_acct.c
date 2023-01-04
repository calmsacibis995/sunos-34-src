/*	@(#)kern_acct.c 1.1 86/09/25 SMI; from UCB 4.1 83/05/27	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/kernel.h"
#include "../h/acct.h"
#include "../h/uio.h"

/*
 * SHOULD REPLACE THIS WITH A DRIVER THAT CAN BE READ TO SIMPLIFY.
 */
struct	vnode *acctp;
struct	vnode *savacctp;

/*
 * Perform process accounting functions.
 */
sysacct(uap)
	register struct a {
		char	*fname;
	} *uap;
{
	struct vnode *vp;

	if (suser()) {
		if (savacctp) {
			acctp = savacctp;
			savacctp = NULL;
		}
		if (uap->fname==NULL) {
			if (vp = acctp) {
				VN_RELE(vp);
				acctp = NULL;
			}
			return;
		}
		u.u_error =
		    lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
			(struct vnode **)0, &vp);
		if (u.u_error)
			return;
		if (vp->v_type != VREG) {
			u.u_error = EACCES;
			VN_RELE(vp);
			return;
		}
		if (acctp)
			VN_RELE(acctp);
		acctp = vp;
	}
}

int	acctsuspend = 2;	/* stop accounting when < 2% free space left */
int	acctresume = 4;		/* resume when free space risen to > 4% */

struct	acct acctbuf;
/*
 * On exit, write a record on the accounting file.
 */
acct()
{
	register int i;
	register struct vnode *vp;
	register struct acct *ap = &acctbuf;
	struct statfs sb;

	if (savacctp) {
		(void)VFS_STATFS(savacctp->v_vfsp, &sb);
		if (sb.f_bavail > (acctresume * sb.f_blocks / 100)) {
			acctp = savacctp;
			savacctp = NULL;
			printf("Accounting resumed\n");
		}
	}
	if ((vp = acctp) == NULL)
		return;
	(void)VFS_STATFS(acctp->v_vfsp, &sb);
	if (sb.f_bavail <= (acctsuspend * sb.f_blocks / 100)) {
		savacctp = acctp;
		acctp = NULL;
		printf("Accounting suspended\n");
		return;
	}
	for (i = 0; i < sizeof (ap->ac_comm); i++)
		ap->ac_comm[i] = u.u_comm[i];
	ap->ac_utime = compress((long)u.u_ru.ru_utime.tv_sec);
	ap->ac_stime = compress((long)u.u_ru.ru_stime.tv_sec);
	ap->ac_etime = compress((long)(time.tv_sec - u.u_start));
	ap->ac_btime = u.u_start;
	ap->ac_uid = u.u_ruid;
	ap->ac_gid = u.u_rgid;
	ap->ac_mem = 0;
	if (i = u.u_ru.ru_utime.tv_sec + u.u_ru.ru_stime.tv_sec)
		ap->ac_mem =
		    (u.u_ru.ru_ixrss + u.u_ru.ru_idrss + u.u_ru.ru_isrss) / i;
	ap->ac_io = compress((long)(u.u_ru.ru_inblock + u.u_ru.ru_oublock));
	if (u.u_ttyp)
		ap->ac_tty = u.u_ttyd;
	else
		ap->ac_tty = NODEV;
	ap->ac_flag = u.u_acflag;
	u.u_error =
	    vn_rdwr(UIO_WRITE, vp, (caddr_t)ap, sizeof(acctbuf), 0,
		UIOSEG_KERNEL, IO_UNIT|IO_APPEND, (int *)0);
}

/*
 * Produce a pseudo-floating point representation
 * with 3 bits base-8 exponent, 13 bits fraction.
 */
compress(t)
	register long t;
{
	register exp = 0, round = 0;

	while (t >= 8192) {
		exp++;
		round = t&04;
		t >>= 3;
	}
	if (round) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return ((exp<<13) + t);
}
