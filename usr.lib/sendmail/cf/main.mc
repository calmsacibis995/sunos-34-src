############################################################
############################################################
#####
#####	SENDMAIL CONFIGURATION FILE FOR "MAIN MACHINES"
#####
#####	You should install this file as /usr/lib/sendmail.cf
#####	if your machine is the main (or only) mail-relaying
#####	machine on your Ethernet.  Then edit the file to
#####	customize it for your network configuration.
#####
#####	See the manual "System Administration for the Sun Workstation".
#####	Look at "Setting Up The Mail Routing System" in the chapter on
#####	Communications.  The Sendmail tutorials in the back of the
#####	manual are also useful.
#####
#####	This config assumes that the domain name is the same as the
#####	host name we are running on.
#####
#####	@(#)main.mc 1.1 86/09/25 SMI
#####		(Derived from various UCB configs)
#####
############################################################
############################################################



############################################################
###	local info
############################################################

# my official hostname
# You have two choices here.  If you want the gateway machine to identify
# itself as the DOMAIN, use this line:
Dj$D.$U
# If you want the gateway machine to appear to be INSIDE the domain, use:
#Dj$w.$D.$U

# major relay mailer
DMuucp

# major relay host
ifdef(`m4GATEWAY',, `define(m4GATEWAY,arpa-gateway)')dnl
DR m4GATEWAY
CR m4GATEWAY

# If you want to pre-load the "mailhosts" then use a line like
# FS /usr/lib/mailhosts
# and then change all the occurences of $%y to be $=S instead.
# Otherwise, the default is to use the hosts.byname map if YP
# is running (or else the /etc/hosts file if no YP).

include(sunbase.m4)

include(uucpm.m4)


############################################################
############################################################
#####
#####		RULESET ZERO
#####
#####	domain.cf has a totally custom Ruleset Zero.
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

# resolve the local hostname to "LOCAL".
R$*<$*$=w.LOCAL>$*	$1<$2LOCAL>$4			thishost.LOCAL
R$*<$*$=w.uucp>$*	$1<$2LOCAL>$4			thishost.uucp
R$*<$*$=w>$*		$1<$2LOCAL>$4			thishost

# Mail addressed explicitly to the domain gateway (us)
R$*<@LOCAL>		$@$>30$1			strip our name, retry
R<@LOCAL>:$+		$@$>30$1			retry after route strip

# deliver to known ethernet hosts explicitly specified in our domain
R$*<@$*$%y.LOCAL>$*	$#ether $@$3 $:$1@$2$3.$D.$U$4	user@etherhost.sun.uucp

# etherhost.uucp is treated as etherhost.$D.$U for now.
# This allows them to be addressed from uucpnet as foo!sun!etherhost!user.
R$*<@$*$%y.uucp>$*	$#ether $@$3 $:$1@$2$3.$D.$U$4	user@etherhost.uucp

# Explicitly specified names in our domain -- that we've never heard of
R$*<@$*.LOCAL>$*	$#error $:Never heard of host $2 in domain $D.$U

# Clean up addresses for external use -- kills LOCAL, route-addr ,=>: and etc.
R$*			$:$>9 $1			Then continue...

# resolve UUCP domain
R<@$-.uucp>:$+		$#uucp  $@$1 $:$2		@host.uucp:...
R$+<@$-.uucp>		$#uucp  $@$2 $:$1		user@host.uucp

# Pass Arpanet and Bitnet names up the ladder to our forwarder
R$*<@$*$-.arpa>$*	$#$M    $@$R $:$1@$2$3.arpa$4	user@anything.arpa
R$*<@$*$-.bitnet>$*	$#$M    $@$R $:$1@$2$3.bitnet$4	user@anything.bitnet

# All addresses in the rules ABOVE are absolute (fully qualified domains).
# (Note that all patterns end in a word, not a multi-matcher).
# Addresses BELOW can be partially qualified.

# deliver to known ethernet hosts
R$*<@$*$%y>$*		$#ether $@$3 $:$1@$2$3$4	user@etherhost

# other non-local names have nowhere to go; return them to sender.
R$*<@$+.$->$*		$#error $:Unknown domain $3
R$*<@$+>$*		$#error $:Never heard of $2; maybe you mean $2.arpa ?
R$*@$*			$#error $:I don't understand $1@$2

# Local names with % are really not local!
R$+%$+			$@$>30$1@$2			turn % => @, retry

# everything else is a local name
R$+			$#local $:$1			local names
