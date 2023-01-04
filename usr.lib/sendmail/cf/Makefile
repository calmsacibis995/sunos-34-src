######################################################################
#
#	Makefile for Sendmail configuration files
#
#	@(#)Makefile 1.1 86/09/25 SMI; from UCB 3.20	5/1/83
#
######################################################################

UWSRCS=	dbvax.mc
SUNSRCS=main.mc \
	subsidiary.mc \
	sun.mc
GENSRCS=arpaproto.mc \
	base.m4 \
	berkm.m4 \
	cchosts.m4 \
	compat.m4 \
	ether.m4 \
	etherm.m4 \
	fudge.m4 \
	ibmm.m4 \
	localm.m4 \
	mmdfm.m4 \
	suucpm.m4 \
	tcpm.m4 \
	uucpm.m4 \
	uucpproto.mc \
	version.m4 \
	zerobase.m4
SRCS=	$(GENSRCS) $(UWSRCS)

UWALL=	dbvax.cf
SUNALL=	main.cf \
	subsidiary.cf \
	sun.cf
PROTOS=	arpaproto.cf uucpproto.cf
ALL=	$(SUNALL)

GET=	sccs get

.SUFFIXES: .mc .cf

.mc.cf:
	m4 $*.mc > $*.cf

all: $(ALL)

############################
#   special dependencies   #
############################

# all Sun hosts
$(SUNALL): sunbase.m4

# pessimize -- basic dependencies
$(ALL): ether.m4 etherm.m4 base.m4 localm.m4 tcpm.m4 version.m4 zerobase.m4

# hosts using UUCP
main.cf subsidiary.cf sun.cf ucbvax.cf uucpproto.cf: uucpm.m4 compat.m4
main.cf subsidiary.cf sun.cf arpa.cf: suucpm.m4

# hosts using MMDF
main.cf sun.cf arpa.cf: mmdfm.m4

# hosts knowing about berknet
jade.cf ucbvax.cf: cchosts.m4 berkm.m4 compat.m4 berkhosts.m4

# cross-mc file dependencies
sun.cf: main.mc

#####################
#   administrivia   #
#####################

sources: $(SRCS)

$(SRCS):
	$(GET) $(REL) SCCS/s.$@

clean:
	rm -f $(ALL) a.out core make.out errs
	rm -f ,*
