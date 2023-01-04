############################################################
#
#	General configuration information
#
#	This information is basically just "boiler-plate"; it must be
#	there, but seldom needs tweaking.
#
#	@(#)base.m4 1.1 86/09/25 SMI; from UCB	3.54	6/11/83
#
############################################################

include(version.m4)

##########################
###   Special macros   ###
##########################

# my name
DnMAILER-DAEMON
# UNIX header format
DlFrom $g  $d
# delimiter (operator) characters
Do.:%@!^=/[]
# format of a total name
Dq$g$?x ($x)$.
# SMTP login message
De$j Sendmail $v/$V ready at $b

###################
###   Options   ###
###################

# location of alias file
OA/usr/lib/aliases
# default delivery mode (deliver in background)
Odbackground
# (don't) connect to "expensive" mailers
#Oc
# rebuild the alias file automagically
OD
# temporary file mode -- 0600 for secure mail, 0644 for permissive
OF0600
# default GID
Og1
# location of help file
OH/usr/lib/sendmail.hf
# log level
OL9
# default messages to old style
Oo
# Cc my postmaster on error replies I generate
OPPostmaster
# queue directory
OQ/usr/spool/mqueue
# read timeout -- violates protocols
Or30m
# status file -- none
OS
# queue up everything before starting transmission, for safety
Os
# default timeout interval
OT3d
# default UID
Ou1

###############################
###   Message precedences   ###
###############################

Pfirst-class=0
Pspecial-delivery=100
Pjunk=-100

#########################
###   Trusted users   ###
#########################

Troot
Tdaemon
Tuucp

#############################
###   Format of headers   ###
#############################

H?P?Return-Path: <$g>
HReceived: $?sfrom $s $.by $j ($v/$V)
	id $i; $b
H?D?Resent-Date: $a
H?D?Date: $a
H?F?Resent-From: $q
H?F?From: $q
H?x?Full-Name: $x
HSubject:
# HPosted-Date: $a
# H?l?Received-Date: $b
H?M?Resent-Message-Id: <$t.$i@$j>
H?M?Message-Id: <$t.$i@$j>

###########################
###   Rewriting rules   ###
###########################


################################
#  Sender Field Pre-rewriting  #
################################
S1
# None needed.

###################################
#  Recipient Field Pre-rewriting  #
###################################
S2
# None needed.

#################################
#  Final Output Post-rewriting  #
#################################

S4
R$+<@$+.uucp>		$2!$1				u@h.uucp => h!u
R$+			$: $>9 $1			Clean up addr
R$*<$+>$*		$1$2$3				defocus


################################################
#  Clean up an address for passing to a mailer #
#  (but leave it focused)		       #
################################################

S9
# externalize internal forms which don't meet external specs
R@			$@$n				handle <> error addr
R$*<$*LOCAL>$*		$1<$2$D.$U>$3			change local info
R<@$+>$*:$+:$+		<@$1>$2,$3:$4			<route-addr> canonical


###########################
#  Name Canonicalization  #
###########################

# Internal format of addresses within the rewriting rules is:
# 	anything<@host.domain.domain...>anything
# We try to get every kind of address into this format, except for local
# addresses, which have no host part.  The reason for the "<>" stuff is
# that the relevant host name could be on the front of the address (for
# source routing), or on the back (normal form).  We enclose the one that
# we want to route on in the <>'s to make it easy to find.
# 

S3

# handle "from:<>" special case
R<>			$@@				turn into magic token

# basic textual canonicalization
R$*<$+>$*		$2				basic RFC821/822 parsing

# make sure <@a,@b,@c:user@d> syntax is easy to parse -- undone later
R@$+,$+:$+		@$1:$2:$3			change all "," to ":"
R@$+:$+			$@$>6<@$1>:$2			src route is canonical

# more miscellaneous cleanup
R$+			$:$>8$1				host dependent cleanup
R$+:$*;@$+		$@$1:$2;@$3			list syntax
R$+@$+			$:$1<@$2>			focus on domain
R$+<$+@$+>		$1$2<@$3>			move gaze right
R$+<@$+>		$@$>6$1<@$2>			already canonical

# convert old-style addresses to domain-based addresses
# All old-style addresses parse from left to right, without precedence.
# Note that the left side of '%' is a username; it is matched with $+ so
# that complex names like "john.gilmore%l5" will be caught and translated.
# The rest can only have an atom as the host name (left of the symbol).
R$-:$+			$@$>3$2@$1			host:user
R$-^$+			$1!$2				convert ^ to !
R$-!$+			$@$>6$2<@$1.uucp>		uucphost!user
R$-=$+			$@$>6$2<@$1.bitnet>		bitnethost=user
R$-.$+!$+		$@$>6$3<@$1.$2>			host.domain!user
R$+%$+			$@$>3$1@$2			user%host
