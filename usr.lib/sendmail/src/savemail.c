# include <pwd.h>
# include "sendmail.h"

SCCSID(@(#)savemail.c 1.1 86/09/25 SMI); /* from UCB 4.5 5/13/84 */

/*
**  SAVEMAIL -- Save mail on error
**
**	If mailing back errors, mail it back to the originator
**	together with an error message; otherwise, just put it in
**	dead.letter in the user's home directory (if he exists on
**	this machine).
**
**	Parameters:
**		e -- the envelope containing the message in error.
**
**	Returns:
**		none
**
**	Side Effects:
**		Saves the letter, by writing or mailing it back to the
**		sender, or by putting it in dead.letter in her home
**		directory.
*/

savemail(e)
	register ENVELOPE *e;
{
	register struct passwd *pw;
	register FILE *xfile;
	char buf[MAXLINE+1];
	extern struct passwd *getpwnam();
	register char *p;
	char triedmail = 0;		/* We tried to mail it back */
	char trieddead = 0;		/* We tried to put it in dead.letter */
	int savedErrors;
	extern char *ttypath();
	typedef int (*fnptr)();

# ifdef DEBUG
	if (tTd(6, 1))
		printf("\nsavemail\n");
# endif DEBUG

	if (bitset(EF_RESPONSE, e->e_flags))
		return;
	if (e->e_class < 0)
	{
		message(Arpa_Info, "Dumping junk mail");
		return;
	}
	ForceMail = TRUE;
	e->e_flags &= ~EF_FATALERRS;

	/*
	**  In the unhappy event we don't know who to return the mail
	**  to, make someone up.
	*/

	if (e->e_from.q_paddr == NULL)
	{
		if (parseaddr("root", &e->e_from, 0, '\0') == NULL)
		{
			syserr("Cannot parse root!");
			ExitStat = EX_SOFTWARE;
			finis();
		}
	}
	e->e_to = NULL;

	/*
	**  If writing back, do it.
	**	If the user is still logged in on the same terminal,
	**	then write the error messages back to hir (sic).
	**	If not, mail back instead.
	*/

	if (ErrorMode == EM_WRITE)
	{
		p = ttypath();
		if (p == NULL || freopen(p, "w", stdout) == NULL)
		{
			ErrorMode = EM_MAIL;
			errno = 0;
		}
		else
		{
			expand("\001n", buf, &buf[sizeof buf - 1], e);
			printf("\r\nMessage from %s...\r\n", buf);
			printf("Errors occurred while sending mail.\r\n");
			if (e->e_xfp != NULL)
			{
				(void) fflush(e->e_xfp);
				xfile = fopen(queuename(e, 'x'), "r");
			}
			else
				xfile = NULL;
			if (xfile == NULL)
			{
				syserr("Cannot open %s", queuename(e, 'x'));
				printf("Transcript of session is unavailable.\r\n");
			}
			else
			{
				printf("Transcript follows:\r\n");
				while (fgets(buf, sizeof buf, xfile) != NULL &&
				       !ferror(stdout))
					fputs(buf, stdout);
				(void) fclose(xfile);
			}
			if (ferror(stdout))
				(void) syserr("savemail: stdout: write err");
		}
	}

	/*
	**  If mailing back, do it.
	**	Throw away all further output.  Don't do aliases, since
	**	this could cause loops, e.g., if joe mails to x:joe,
	**	and for some reason the network for x: is down, then
	**	the response gets sent to x:joe, which gives a
	**	response, etc.  Also force the mail to be delivered
	**	even if a version of it has already been sent to the
	**	sender.
	*/

	if (ErrorMode == EM_MAIL)
	{
trymail:
		triedmail++;
		if (e->e_errorqueue == NULL)
			sendtolist(e->e_from.q_paddr, (ADDRESS *) NULL,
			       &e->e_errorqueue);
		if (e->e_message == NULL) 
			e->e_message =  newstr("Unable to deliver mail");
	/*
	**  Carbon copy the message to Postmaster (or whoever) if
	**  specified by the "P" config option. 
	*/
		if (PostmasterCopy != 0)
		{
		  ADDRESS *postmaster = NULL;
		  
		  sendtolist(PostmasterCopy, (ADDRESS *) NULL, &postmaster);
		  (void) returntosender(e->e_message, postmaster, FALSE);
		}

		if (returntosender(e->e_message, e->e_errorqueue, TRUE) == 0)
			return;
		/* It didn't work -- if dead.letter didn't either, give up. */
		if (trieddead) return;
	}

	/*
	**  Save the message in dead.letter.
	**	If we weren't mailing back, and the user is local, we
	**	should save the message in dead.letter so that the
	**	poor person doesn't have to type it over again --
	**	and we all know what poor typists programmers are.
	*/

	trieddead++;
	p = NULL;
	if (e->e_from.q_mailer == LocalMailer)
	{
		if (e->e_from.q_home != NULL)
			p = e->e_from.q_home;
		else if ((pw = getpwnam(e->e_from.q_user)) != NULL)
			p = pw->pw_dir;
	}
	if (p == NULL)
	{
		if (!triedmail) goto trymail;
		syserr("Can't return mail to %s", e->e_from.q_paddr);
# ifdef DEBUG
		p = "/usr/tmp";
# endif
	}
	if (p != NULL && e->e_dfp != NULL)
	{
		auto ADDRESS *q;
		bool oldverb = Verbose;

		/* we have a home directory; open dead.letter */
		define('z', p, e);
		expand("\001z/dead.letter", buf, &buf[sizeof buf - 1], e);
		Verbose = TRUE;
		message(Arpa_Info, "Saving message in %s", buf);
		Verbose = oldverb;
		e->e_to = buf;
		q = NULL;
		/*
		**  Attempt to notice when sendtolist fails, by comparing
		**  old and new values of Errors.  This seems to be easiest,
		**  since no status is returned.         -- sun!gnu 30Mar84
		**  Note, we can't just compare them after calling deliver()
		**  since deliver() resets Errors to zero when it starts.
		*/
		savedErrors = Errors;
		sendtolist(buf, (ADDRESS *) NULL, &q);
		savedErrors -= Errors;
		if (deliver(e, q) != 0 || savedErrors != 0)
		{
			/*
			**  Delivering to ~/dead.letter didn't appear to work.
			**  If we haven't already tried mailing the failing
			**  message back to its originator, try that.
			*/
			if (!triedmail)	
				goto trymail;
		}
	}

	/* add terminator to writeback message */
	if (ErrorMode == EM_WRITE)
		printf("-----\r\n");
}
/*
**  RETURNTOSENDER -- return a message to the sender with an error.
**
**	Parameters:
**		msg -- the explanatory message.
**		returnq -- the queue of people to send the message to.
**		sendbody -- if TRUE, also send back the body of the
**			message; otherwise just send the header.
**
**	Returns:
**		zero -- if everything went ok.
**		else -- some error.
**
**	Side Effects:
**		Returns the current message to the sender via
**		mail.
*/

static bool	SendBody;

#define MAXRETURNS	6	/* max depth of returning messages */

returntosender(msg, returnq, sendbody)
	char *msg;
	ADDRESS *returnq;
	bool sendbody;
{
	char buf[MAXNAME];
	extern putheader(), errbody();
	register ENVELOPE *ee;
	extern ENVELOPE *newenvelope();
	ENVELOPE errenvelope;
	static int returndepth;
	register ADDRESS *q;

# ifdef DEBUG
	if (tTd(6, 1))
	{
		printf("Return To Sender: msg=\"%s\", depth=%d, CurEnv=%x,\n",
		       msg, returndepth, CurEnv);
		printf("\treturnq=");
		printaddr(returnq, TRUE);
	}
# endif DEBUG

	if (++returndepth >= MAXRETURNS)
	{
		if (returndepth != MAXRETURNS)
			syserr("returntosender: infinite recursion on %s", returnq->q_paddr);
		/* don't "unrecurse" and fake a clean exit */
		/* returndepth--; */
		return (0);
	}

	SendBody = sendbody;
	define('g', "\001f", CurEnv);
	ee = newenvelope(&errenvelope);
	define('a', "\001b", ee);
	ee->e_puthdr = putheader;
	ee->e_putbody = errbody;
	ee->e_flags |= EF_RESPONSE;
	ee->e_sendqueue = returnq;
	openxscript(ee);
	for (q = returnq; q != NULL; q = q->q_next)
	{
		if (q->q_alias == NULL)
			addheader("to", q->q_paddr, ee);
	}

	(void) sprintf(buf, "Returned mail: %s", msg);
	addheader("subject", buf, ee);

	/* fake up an address header for the from person */
	expand("\001n", buf, &buf[sizeof buf - 1], CurEnv);
	if (parseaddr(buf, &ee->e_from, -1, '\0') == NULL)
	{
		syserr("Can't parse myself!");
		ExitStat = EX_SOFTWARE;
		returndepth--;
		return (-1);
	}
	loweraddr(&ee->e_from);

	/* push state into submessage */
	CurEnv = ee;
	define('f', "\001n", ee);
	define('x', "Mail Delivery Subsystem", ee);
	eatheader(ee);

	/* actually deliver the error message */
	sendall(ee, SM_DEFAULT);

	/* restore state */
	dropenvelope(ee);
	CurEnv = CurEnv->e_parent;
	returndepth--;

	/* should check for delivery errors here */
	return (0);
}
/*
**  ERRBODY -- output the body of an error message.
**
**	Typically this is a copy of the transcript plus a copy of the
**	original offending message.
**
**	Parameters:
**		fp -- the output file.
**		m -- the mailer to output to.
**		e -- the envelope we are working in.
**
**	Returns:
**		none
**
**	Side Effects:
**		Outputs the body of an error message.
*/

errbody(fp, m, e)
	register FILE *fp;
	register struct mailer *m;
	register ENVELOPE *e;
{
	register FILE *xfile;
	char buf[MAXLINE];
	char *p;

	/*
	**  Output transcript of errors
	*/

	(void) fflush(stdout);
	p = queuename(e->e_parent, 'x');
	if ((xfile = fopen(p, "r")) == NULL)
	{
		syserr("Cannot open %s", p);
		fprintf(fp, "  ----- Transcript of session is unavailable -----\n");
	}
	else
	{
		fprintf(fp, "   ----- Transcript of session follows -----\n");
		if (e->e_xfp != NULL)
			(void) fflush(e->e_xfp);
		while (fgets(buf, sizeof buf, xfile) != NULL)
			putline(buf, fp, m);
		(void) fclose(xfile);
	}
	errno = 0;

	/*
	**  Output text of original message
	*/

	if (NoReturn)
		fprintf(fp, "\n   ----- Return message suppressed -----\n\n");
	else if (e->e_parent->e_dfp != NULL)
	{
		if (SendBody)
		{
			putline("\n", fp, m);
			putline("   ----- Unsent message follows -----\n", fp, m);
			(void) fflush(fp);
			putheader(fp, m, e->e_parent);
			putline("\n", fp, m);
			putbody(fp, m, e->e_parent);
		}
		else
		{
			putline("\n", fp, m);
			putline("  ----- Message header follows -----\n", fp, m);
			(void) fflush(fp);
			putheader(fp, m, e->e_parent);
		}
	}
	else
	{
		putline("\n", fp, m);
		putline("  ----- No message was collected -----\n", fp, m);
		putline("\n", fp, m);
	}

	/*
	**  Cleanup and exit
	*/

	if (errno != 0)
		syserr("errbody: I/O error");
}
