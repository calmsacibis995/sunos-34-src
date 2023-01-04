############################################################
############################################################
#####
#####		Ethernet Mailer specification
#####
#####	Messages processed by this configuration are assumed to remain
#####	in the same domain.  Hence, they may not necessarily correspond
#####	to RFC822 in all details.
#####
#####		@(#)etherm.m4 1.1 86/09/25 SMI; from UCB	4.3	11/13/84
#####
#####		(This should really be called the TCP mailer, since
#####		 nothing here is particular to Ethernet)
############################################################
############################################################

Mether,	P=[IPC], F=msDFMuCX, S=11, R=21, A=IPC $h
S11
R$*<@$+>$*		$@$1<@$2>$3			already ok
R$+			$@$1<@$w>			tack on our hostname

S21
# None needed.
