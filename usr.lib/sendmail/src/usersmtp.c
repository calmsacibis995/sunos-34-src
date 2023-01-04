# include <ctype.h>
# include <sysexits.h>
# include <errno.h>
# include "sendmail.h"

# ifndef SMTP
SCCSID(@(#)usersmtp.c 1.1 86/09/25 SMI (no SMTP)); /* from UCB 4.11 4/4/85 */
# else SMTP

SCCSID(@(#)usersmtp.c 1.1 86/09/25 SMI); /* from UCB 4.11 4/4/85 */



/*
**  USERSMTP -- run SMTP protocol from the user end.
**
**	This protocol is described in RFC821.
*/

#define REPLYTYPE(r)	((r) / 100)		/* first digit of reply code */
#define REPLYCLASS(r)	(((r) / 10) % 10)	/* second digit of reply code */
#define SMTPCLOSING	421			/* "Service Shutting Down" */

char	SmtpMsgBuffer[MAXLINE];		/* buffer for commands */
char	SmtpReplyBuffer[MAXLINE];	/* buffer for replies */
char	SmtpError[MAXLINE] = "";	/* for failure error messages */
char 	*SmtpPhase = NULL;		/* for informative state info */
FILE	*SmtpOut;			/* output file */
FILE	*SmtpIn;			/* input file */
int	SmtpPid;			/* pid of mailer */
int	SmtpFirstErrno;			/* Meaningful error number */

/* following represents the state of the SMTP connection */
int	SmtpState;			/* connection state, see below */

#define SMTP_CLOSED	0		/* connection is closed */
#define SMTP_OPEN	1		/* connection is open for business */
#define SMTP_SSD	2		/* service shutting down */
#define	SMTP_WAITING	3		/* Open, waiting for HELO */

#define	GREETTIMEOUT	(time_t)300	/* seconds to wait for greeting */
#define	QUITTIMEOUT	(time_t)300	/* seconds for response to QUIT */
/*
**  SMTPINIT -- initialize SMTP.
**
**	Opens the connection and sends the initial protocol.
**
**	Parameters:
**		m -- mailer to create connection to.
**		pvp -- pointer to parameter vector to pass to
**			the mailer.
**
**	Returns:
**		appropriate exit status -- EX_OK on success.
**		If not EX_OK, it should close the connection.
**
**	Side Effects:
**		creates connection and sends initial protocol.
*/

smtpinit(m, pvp)
	struct mailer *m;
	char **pvp;
{
	register int r;
	EVENT *gte;
	char buf[MAXNAME];

	/*
	**  Open the connection to the mailer.
	*/

#ifdef DEBUG
	if (SmtpState == SMTP_OPEN)
		syserr("smtpinit: already open");
#endif DEBUG

	SmtpIn = SmtpOut = NULL;
	SmtpFirstErrno = 0;	
	SmtpState = SMTP_CLOSED;
	SmtpPhase = "initiation";
	(void) strcpy(SmtpError, "");
	SmtpPid = openmailer(m, pvp, (ADDRESS *) NULL, TRUE, &SmtpOut, &SmtpIn);
	if (SmtpPid < 0)
	{
# ifdef DEBUG
		if (tTd(18, 1))
			printf("smtpinit: cannot open %s: stat %d errno %d\n",
			   pvp[0], ExitStat, errno);
# endif DEBUG
		if (CurEnv->e_xfp != NULL)
		{
			extern char *errstring();

			if (ExitStat==EX_NOHOST)
			    fprintf(CurEnv->e_xfp,
				"421 Host %s not found for mailer %s.\n",
				pvp[1], m->m_name);
			else 
			  {
			     extern char *pintvl();
			     long timeleft;

			     fprintf(CurEnv->e_xfp,
				"421 %s: %s", CurEnv->e_to, errstring(errno));
			     timeleft = CurEnv->e_ctime + TimeOut - curtime();
			     timeleft = (timeleft+1800)/3600;
			     if (timeleft > 0)
			       fprintf(CurEnv->e_xfp,
			         ", will keep trying for %s",
				 pintvl(timeleft*3600,FALSE) );
			     fprintf(CurEnv->e_xfp,"\n");
			  }
		}
		return (ExitStat);
	}
	SmtpState = SMTP_WAITING;
	SmtpPhase = "greeting wait";

	/*
	**  Get the greeting message.
	**	This should appear spontaneously.  Give it five minutes to
	**	happen.
	*/

	r = reply(m, GREETTIMEOUT);
	if (r < 0 || REPLYTYPE(r) != 2)
		goto tempfail;
	SmtpState = SMTP_OPEN;
	SmtpPhase = "hello wait";

	/*
	**  Send the HELO command.
	**	My mother taught me to always introduce myself.
	*/

	smtpmessage("HELO %s", m, HostName);
	r = reply(m, ReadTimeout);
	if (r < 0)
		goto tempfail;
	else if (REPLYTYPE(r) == 5)
		goto unavailable;
	else if (REPLYTYPE(r) != 2)
		goto tempfail;

	/*
	**  If this is expected to be another sendmail, send some internal
	**  commands.
	*/

	if (bitnset(M_INTERNAL, m->m_flags))
	{
		/* tell it to be verbose */
		smtpmessage("VERB", m);
		r = reply(m, ReadTimeout);
		if (r < 0)
			goto tempfail;

		/* tell it we will be sending one transaction only */
		smtpmessage("ONEX", m);
		r = reply(m, ReadTimeout);
		if (r < 0)
			goto tempfail;
	}

	/*
	**  Send the MAIL command.
	**	Designates the sender.
	*/

	expand("\001g", buf, &buf[sizeof buf - 1], CurEnv);
	if (CurEnv->e_from.q_mailer == LocalMailer ||
	    !bitnset(M_FROMPATH, m->m_flags))
	{
		smtpmessage("MAIL From:<%s>", m, buf);
	}
	else
	{
		smtpmessage("MAIL From:<@%s%c%s>", m, HostName,
			buf[0] == '@' ? ',' : ':', buf);
	}
	SmtpPhase = "\"mail from\" wait";
	r = reply(m, ReadTimeout);
	if (r < 0 || REPLYTYPE(r) == 4)
		goto tempfail;
	else if (r == 250)
		return (EX_OK);
	else if (r == 552)
		goto unavailable;

	/* protocol error -- close up */
	smtpquit(m);
	return (EX_PROTOCOL);

	/* signal a temporary failure */
  tempfail:
	smtpquit(m);
	if (SmtpFirstErrno) errno = SmtpFirstErrno;
	return (EX_TEMPFAIL);

	/* signal service unavailable */
  unavailable:
	smtpquit(m);
	return (EX_UNAVAILABLE);
}
/*
**  SMTPRCPT -- designate recipient.
**
**	Parameters:
**		to -- address of recipient.
**		m -- the mailer we are sending to.
**
**	Returns:
**		exit status corresponding to recipient status.
**
**	Side Effects:
**		Sends the mail via SMTP.
*/

smtprcpt(to, m)
	ADDRESS *to;
	register MAILER *m;
{
	register int r;
	extern char *remotename();

	smtpmessage("RCPT To:<%s>", m, remotename(to->q_user, m, FALSE, TRUE));

	SmtpPhase = "\"rcpt to\" wait";
	r = reply(m, ReadTimeout);
	if (r < 0 || REPLYTYPE(r) == 4)
	  {
		if (SmtpFirstErrno) errno = SmtpFirstErrno;
		return (EX_TEMPFAIL);
	  }
	else if (REPLYTYPE(r) == 2)
		return (EX_OK);
	else if (r == 550 || r == 551 || r == 553)
		return (EX_NOUSER);
	else if (r == 552 || r == 554)
		return (EX_UNAVAILABLE);
	return (EX_PROTOCOL);
}
/*
**  SMTPDATA -- send the data and clean up the transaction.
**
**	Parameters:
**		m -- mailer being sent to.
**		e -- the envelope for this message.
**
**	Returns:
**		exit status corresponding to DATA command.
**
**	Side Effects:
**		none.
*/

smtpdata(m, e)
	struct mailer *m;
	register ENVELOPE *e;
{
	register int r;

	/*
	**  Send the data.
	**	First send the command and check that it is ok.
	**	Then send the data.
	**	Follow it up with a dot to terminate.
	**	Finally get the results of the transaction.
	*/

	/* send the command and check ok to proceed */
	smtpmessage("DATA", m);
	SmtpPhase = "data wait";
	r = reply(m, ReadTimeout);
	if (r < 0 || REPLYTYPE(r) == 4)
	  {
		if (SmtpFirstErrno) errno = SmtpFirstErrno;
		return (EX_TEMPFAIL);
	  }
	else if (r == 554)
		return (EX_UNAVAILABLE);
	else if (r != 354)
		return (EX_PROTOCOL);

	/* now output the actual message */
	(*e->e_puthdr)(SmtpOut, m, CurEnv);
	putline("\n", SmtpOut, m);
	(*e->e_putbody)(SmtpOut, m, CurEnv);

	/* terminate the message */
	fprintf(SmtpOut, ".%s", m->m_eol);
	if (Verbose && !HoldErrs)
		nmessage(Arpa_Info, ">>> .");

	/* check for the results of the transaction */
	SmtpPhase = "result wait";
	r = reply(m, ReadTimeout);
	if (r < 0 || REPLYTYPE(r) == 4)
	  {
		if (SmtpFirstErrno) errno = SmtpFirstErrno;
		return (EX_TEMPFAIL);
	  }
	else if (r == 250)
		return (EX_OK);
	else if (r == 552 || r == 554)
		return (EX_UNAVAILABLE);
	return (EX_PROTOCOL);
}
/*
**  SMTPQUIT -- close the SMTP connection.
**
**	Parameters:
**		m -- a pointer to the mailer.
**
**	Returns:
**		none.
**
**	Side Effects:
**		sends the final protocol and closes the connection.
*/

smtpquit(m)
	register MAILER *m;
{
	int i;

	/* if the connection is already closed, don't bother */
	if (SmtpIn == NULL)
		return;

	/* send the quit message if not a forced quit */
	if (SmtpState == SMTP_OPEN || SmtpState == SMTP_SSD)
	{
		smtpmessage("QUIT", m);
		(void) reply(m, QUITTIMEOUT);
		if (SmtpState == SMTP_CLOSED)
			return;
	}

	/* now actually close the connection */
	closeconnection(fileno(SmtpIn));	/* Mark closed in stab */
	(void) fclose(SmtpIn);
	(void) fclose(SmtpOut);
	SmtpIn = SmtpOut = NULL;
	SmtpState = SMTP_CLOSED;

	/* and pick up the zombie */
	i = endmailer(SmtpPid, m->m_argv[0]);
	if (i != EX_OK)
		syserr("smtpquit %s: stat %d", m->m_argv[0], i);
}
/*
**  REPLY -- read arpanet reply
**
**	Parameters:
**		m -- the mailer we are reading the reply from.
**		timeout -- how long to wait for a reply.
**
**	Returns:
**		reply code it reads.
**
**	Side Effects:
**		flushes the mail file.
*/

reply(m, timeout)
	MAILER *m;
	time_t timeout;
{
	if (SmtpOut == NULL || SmtpIn == NULL) 
		return(SMTPCLOSING);
	(void) fflush(SmtpOut);

# ifdef DEBUG
	if (tTd(18, 9))
	  {
	  	char buf[512];
		printf("inside reply, hit return:");
		gets(buf);
	  }
# endif DEBUG

	/*
	**  Read the input line, being careful not to hang.
	*/

	for (;;)
	{
		register int r;
		register char *p;

		/* actually do the read */
		if (CurEnv->e_xfp != NULL)
			(void) fflush(CurEnv->e_xfp);	/* for debugging */

		/* if we are in the process of closing just give the code */
		if (SmtpState == SMTP_CLOSED)
			return (SMTPCLOSING);

		/* get the line from the other side */
		p = sfgets(SmtpReplyBuffer, sizeof SmtpReplyBuffer, SmtpIn,
			timeout);
		if (p == NULL)
		{
			  /*
			   * Make sure we produce a temporary error
			   * if the remote end just closed early.
			   */
			 if (errno==0)
			 	errno = ECONNRESET;
			if (SmtpFirstErrno==0)
				SmtpFirstErrno = errno;
			syserr("network read error");
# ifdef DEBUG
			/* if debugging, pause so we can see state */
			if (tTd(18, 100))
				pause();
# endif DEBUG
			SmtpState = SMTP_CLOSED;
			smtpquit(m);
			return (-1);
		}
		fixcrlf(SmtpReplyBuffer, TRUE);

		if (CurEnv->e_xfp != NULL && index("45", SmtpReplyBuffer[0]) != NULL)
		{
			/* serious error -- log the previous command */
			if (SmtpMsgBuffer[0] != '\0')
				fprintf(CurEnv->e_xfp, ">>> %s\n", SmtpMsgBuffer);
			SmtpMsgBuffer[0] = '\0';

			/* now log the message as from the other side */
			fprintf(CurEnv->e_xfp, "<<< %s\n", SmtpReplyBuffer);
		}

		/* display the input for verbose mode */
		if (Verbose && !HoldErrs)
			nmessage(Arpa_Info, "%s", SmtpReplyBuffer);

		/* if continuation is required, we can go on */
		if (SmtpReplyBuffer[3] == '-' || !isdigit(SmtpReplyBuffer[0]))
			continue;

		/* decode the reply code */
		r = atoi(SmtpReplyBuffer);

		/* extra semantics: 0xx codes are "informational" */
		if (r < 100)
			continue;

		if (SmtpReplyBuffer[0] == '4' && SmtpError[0] == '\0')
			(void) strcpy(SmtpError, SmtpReplyBuffer+4);

		/* reply code 421 is "Service Shutting Down" */
		if (r == SMTPCLOSING && SmtpState != SMTP_SSD)
		{
			/* send the quit protocol */
			SmtpState = SMTP_SSD;
			smtpquit(m);
		}

		return (r);
	}
}
/*
**  SMTPMESSAGE -- send message to server
**
**	Parameters:
**		f -- format
**		m -- the mailer to control formatting.
**		a, b, c -- parameters
**
**	Returns:
**		none.
**
**	Side Effects:
**		writes message to SmtpOut.
*/

/*VARARGS1*/
smtpmessage(f, m, a, b, c)
	char *f;
	MAILER *m;
{
	(void) sprintf(SmtpMsgBuffer, f, a, b, c);
	if (tTd(18, 1) || (Verbose && !HoldErrs))
		nmessage(Arpa_Info, ">>> %s", SmtpMsgBuffer);
	if (SmtpOut != NULL)
		fprintf(SmtpOut, "%s%s", SmtpMsgBuffer, m->m_eol);
}

# endif SMTP
