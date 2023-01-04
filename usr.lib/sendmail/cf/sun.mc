############################################################
############################################################
#####
#####	SENDMAIL CONFIGURATION FILE FOR HOST "sun"
#####
#####	@(#)sun.mc 1.1 86/09/25 SMI
#####
############################################################
############################################################

#
# Sun is a pretty normal domain, but wants to mess with Arpanet
# addresses being relayed to it by Berkeley.
#
define(m4GATEWAY,ucbvax)dnl
include(main.mc)

#
# Host-dependent address cleanup:  strip Berkeley! relay off Arpanet mail
#
S8
#
# Handle from addresses that arrive over uucp from our Arpanet relay
# site.  They are passed to us in the -f argument from rmail, when uucp
# hands us the message.  Since many Arpa hosts are not qualifying their
# domains yet, we have to tack on ".arpa" if no domain is specified.
#
# If there's no atsign in the name, just let it on thru -- it's being
# relayed from a uucp site.
#
# THIS IS A KLUDGE OF THE EMERGENCY BROADCAST SYSTEM.  THIS IS ONLY A KLUDGE.
# It will have to stay around until our relay site passes us real
# domain-based addresses, at which point all we have to do is strip off
# the uucp garbage on the front.
#
R$=R!$*			$@ $>38 $2		Run thru 38 if came from relay!
# Otherwise, the address is OK as it stands.

# This address is from our Arpa relay site.  Fix it for slow Arpanauts.
S38
R$*			$: $>3 $1		Canonicalize, focus rest of it
R$*<@$-.uucp>		$@ $2!$1@$R.uucp	If uucp addr, leave relay on.
R$*<@$*LOCAL>$*		$@ @$R.uucp:$1@$2$D.$U$3	If LOCAL, deloc&relay
R$*<@$+.$*>$*		$@ $1@$2.$3$4		If domain known, defocus&return
R$*<@$+>$*		$@ $1@$2.arpa$3		If not, "arpa", defocus&return
R$*<$*>$*		$@ $R!$1$2$3		Restore unknown strangenesses
R$*			$@ $R!$1		Restore unknown strangenesses

# Kludge to deal with non-UGLYUUCP mail from our relay site.
# Adds host "somewhere" to the class containing our relay site.  It will
# be translated to the real name, or removed, by the S8 code.
CRsomewhere
