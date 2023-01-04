############################################################
############################################################
#####
#####		RULESET ZERO PREAMBLE
#####
#####	The beginning of ruleset zero is constant for many
#####	configurations.
#####
#####	@(#)zerobase.m4 1.1 86/09/25 SMI; from UCB	4.2	11/13/84
#####
############################################################
############################################################

# Ruleset 30 just calls rulesets 3 then 0.
S30
R$*			$: $>3 $1			First canonicalize
R$*			$@ $>0 $1			Then rerun ruleset 0

S0
# On entry, the address has been canonicalized and focused by ruleset 3.
# Handle special cases.....
R@			$#local $:$n			handle <> form
# For numeric spec, you can't pass spec on to receiver, since rcvr's
# are not smart enough to know that [x.y.z.a] is their own name.
R<@[$+]>:$*		$:$>9 <@[$1]>:$2		Clean it up, then...
R<@[$+]>:$*		$#ether $@[$1] $:$2		numeric internet spec
R<@[$+]>,$*		$#ether $@[$1] $:$2		numeric internet spec
R$*<@[$+]>		$#ether $@[$2] $:$1		numeric internet spec

# arrange for local names to be fully qualified
# This is a bug for nested domains.  This should have been done earlier
# by canonicalization...
R$*<@$*$%y>$*		$1<@$2$3.LOCAL>$4		user@etherhost

# now delete redundant local info
R$*<$*$=w.LOCAL>$*	$1<$2>$4			thishost.LOCAL
R$*<@LOCAL>$*		$1<@$D.$U>$2			host == domain gateway
R$*<$*$=w.uucp>$*	$1<$2>$4			thishost.uucp
R$*<$*$=w>$*		$1<$2>$4			thishost
R$*<$*.>$*		$1<$2>$3			drop trailing dot
R<@>:$*			$@$>30$1			retry after route strip
R$*<@>			$@$>30$1			strip null trash & retry
