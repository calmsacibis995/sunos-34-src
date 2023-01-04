#################################################
#
#	General configuration information
#
#	@(#)sunbase.m4 1.1 86/09/25 SMI; from UCB csbase.m4 4.17	11/13/84
#
#################################################

######################
#   General Macros   #
######################

# local domain names
#
# These can now be set from the domainname system call.
# If your YP domain is same as the domain name you would like to have
# appear in your mail headers, then comment out the following two lines.
# Otherwise, edit them to be your mail domain name.
DDsun
CDsun

# domain-spec for local domain within universe (eg, what domains are above?)
DUcom
CUuucp com

# known hosts in this domain are obtained from gethostbyname() call

include(base.m4)

#######################
#   Rewriting rules   #
#######################

##### special local conversions
S6
R$*<@$*$=D>$*		$1<@$2LOCAL>$4			convert local domain
R$*<@$*$=D.$=U>$*	$1<@$2LOCAL>$5			or full domain name

include(localm.m4)
include(etherm.m4)
