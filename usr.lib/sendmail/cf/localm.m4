############################################################
############################################################
#####
#####		Local and Program Mailer specification
#####
#####		@(#)localm.m4 1.1 86/09/25 SMI; from UCB	3.7	5/3/83
#####
############################################################
############################################################

Mlocal,	P=/bin/mail, F=rlsDFMmnP, S=10, R=20, A=mail -d $u
Mprog,	P=/bin/sh,   F=lsDFMeuP,  S=10, R=20, A=sh -c $u

S10
# None needed.

S20
# None needed.
