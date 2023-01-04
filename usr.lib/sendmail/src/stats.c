# include "sendmail.h"
# include "stats.h"

SCCSID(@(#)stats.c 1.1 86/09/25 SMI); /* from UCB 4.2 8/11/84 */

struct statistics	Stat;

/*
**  KBYTES -- given a number, returns the number of Kbytes.
**
**	Used in statistics gathering of message sizes to try to avoid
**	wraparound (at least for a while.....)
**
**	Parameters:
**		bytes -- actual number of bytes.
**
**	Returns:
**		number of kbytes.
**
**	Side Effects:
**		none.
**
**	Notes:
**		This function is actually a ceiling function to
**			the nearest K.
**		Honestly folks, floating point might be better.
**			Or perhaps a "statistical" log method.
**		Changed 4Mar84 by sun!gnu to use "binary K" instead
**			of "decimal K".  Who would ever expect a computer
**			program to print "34K" and mean 34,000?
**		Also changed to a #define for speed.
*/
#define kbytes(bytes) (((bytes) + 1023) >> 10);

/*
**  MARKSTATS -- mark statistics
*/

markstats(e, to)
	register ENVELOPE *e;
	register ADDRESS *to;
{
	if (to == NULL)
	{
		Stat.stat_nf[e->e_from.q_mailer->m_mno]++;
		Stat.stat_bf[e->e_from.q_mailer->m_mno] += kbytes(CurEnv->e_msgsize);
	}
	else
	{
		Stat.stat_nt[to->q_mailer->m_mno]++;
		Stat.stat_bt[to->q_mailer->m_mno] += kbytes(CurEnv->e_msgsize);
	}
}


/*
**  POSTSTATS -- post statistics in the statistics file
**
**	Parameters:
**		sfile -- the name of the statistics file,
**			 or a pointer to an empty string if no statistics
**			 collection is desired.
**
**	Returns:
**		none.
**
**	Side Effects:
**		merges the Stat structure with the sfile file.
*/

poststats(sfile)
	char *sfile;
{
	register int fd;
	struct statistics stat;
	extern long lseek();

	if (sfile == NULL || *sfile == '\0') return;

	(void) time(&Stat.stat_itime);
	Stat.stat_size = sizeof Stat;

	fd = open(sfile, 2);
	if (fd < 0)
	{
		errno = 0;
		return;
	}
	if (read(fd, (char *) &stat, sizeof stat) == sizeof stat &&
	    stat.stat_size == sizeof stat)
	{
		/* merge current statistics into statfile */
		register int i;

		for (i = 0; i < MAXMAILERS; i++)
		{
			stat.stat_nf[i] += Stat.stat_nf[i];
			stat.stat_bf[i] += Stat.stat_bf[i];
			stat.stat_nt[i] += Stat.stat_nt[i];
			stat.stat_bt[i] += Stat.stat_bt[i];
		}
	}
	else
		bcopy((char *) &Stat, (char *) &stat, sizeof stat);

	/* write out results */
	(void) lseek(fd, 0L, 0);
	(void) write(fd, (char *) &stat, sizeof stat);
	(void) close(fd);
}
