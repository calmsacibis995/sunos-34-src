############################################################
############################################################
#####
#####		BASIC ETHERNET RULES
#####
#####		@(#)ether.m4 1.1 86/09/25 SMI; from UCB	4.1	7/25/83
#####
############################################################
############################################################



include(zerobase.m4)

################################################
###  Machine dependent part of ruleset zero  ###
################################################

# resolve names that can go via the ethernet
R$*<@$*$%y.LOCAL>$*	$#ether$@$3$:$1<@$2$3.$D>$4	user@etherhost

# other non-local names will be kicked upstairs
R$*<@$+>$*		$#ether$@$R$:$1<@$2>$3		user@some.where

# remaining names must be local
R$+			$#local$:$1			everything else
