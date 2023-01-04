############################################################
############################################################
#####
#####		Provide Backward Compatibility
#####
#####		@(#)compat.m4 1.1 86/09/25 SMI; from UCB	3.3	2/24/83
#####
############################################################
############################################################

define(m4COMPAT, 1.1)

##########################################################
#  General code to convert back to old style UUCP names  #
##########################################################

S5
R$+<@LOCAL>		$@ $D!$1			name@LOCAL => sun!name
R$+<@$-.LOCAL>		$@ $2!$1			u@h.LOCAL => h!u
R$+<@$+.uucp>		$@ $2!$1			u@h.uucp => h!u
R$+<@$%y>		$@ $2!$1			u@etherh => etherh!u
R$+<@$+>		$@ $1%$2			u@any => u%any
# Route-addrs do not work here.  Punt til uucp-mail comes up with something.
R<@$+>$*		$@ @$1$2			just defocus and punt
R$*<$*>$*		$@ $1$2$3			Defocus strange stuff

