# @(#) Makefile 1.1 9/25/86
#
# Makefile for the document template "doctemplate.msun".
#
# INSTRUCTIONS FOR MAKING A DOCUMENT:
# -----------------------------------
#   (1) The value of PREFIX (see below) MUST be supplied on the command
#       line in the form of  PREFIX=x  where "x" is replaced with the
#       prefix(es) from the file name containing the document which is
#       to be made.  For example, if the document to be made is contained
#       in file "the.great.doc.msun", the command line must read as follows:
#    
#         make [-f file] PREFIX=the.great.doc
#
#       WARNING: Remember that all file names containing a document to
#                be made MUST have the ".msun" suffix.
#
#   (2) If the document to be made has any equations or pictures in it,
#       the following two command line arguments MUST ALSO be provided
#       in addition to the PREFIX argument mentioned above.
#
#         (a) FORMATTER=iroff
#         (b) SUFFIX=dit
#
#       For example, assuming that the document to be made resides in
#       file "the.great.doc.msun", the command line must read as follows:
#
#         make [-f file] PREFIX=the.great.doc FORMATTER=iroff SUFFIX=dit
#
#       The side effects of using "iroff" are that, (1) "tseetool" can
#       NOT be used to preview the document prior to printing it and (2)
#       the following command MUST be used to print the document
#       "lpr -n -P1 file" (rather than "lpr -t -P1 file" which is used 
#       to print "itroff" documents).  
#
# INSTRUCTIONS FOR CLEANING UP AFTER THE DOCUMENT HAS BEEN PRINTED:
# -----------------------------------------------------------------
#   (1) Assuming that the document which was made resides in file
#       "the.great.doc.msun", the command line MUST read as follows:
#
#         make [-f file] PREFIX=the.great.doc clean
#
#       Notice that, file "the.great.doc.msun will NOT be removed
#       by the clean up process.
#

PREFIX =
SUFFIX = cat
FORMATTER = itroff
PICTURES = /usr/local/pic -Timp
EQUATIONS = /usr/local/eqn
FORMTR = /usr/local/$(FORMATTER)
MACRO_PACKAGE = msun
CONTENTSPAGE = 1

all: $(PREFIX).$(SUFFIX) $(PREFIX)tc

$(PREFIX).$(SUFFIX) $(PREFIX)tc.msun: $(PREFIX).msun
	soelim $(PREFIX).msun | $(PICTURES) | tbl | $(EQUATIONS) | $(FORMTR) -t -$(MACRO_PACKAGE) \
	> $(PREFIX).$(SUFFIX) 2>$(PREFIX)tc.msun

$(PREFIX)tc: $(PREFIX)tc.msun
	echo .UH CONTENTS > $(PREFIX)Pages.msun
	grep '^\.CE [CANU]' $(PREFIX)tc.msun | \
		$(FORMTR) -t -rP$(CONTENTSPAGE) -$(MACRO_PACKAGE) \
		$(PREFIX)Pages.msun - > $(PREFIX)tc.$(SUFFIX)

clean:
	rm -f $(PREFIX)tc.msun $(PREFIX)Pages.msun 
	rm -f $(PREFIX)tc.cat $(PREFIX).cat
	rm -f $(PREFIX)tc.dit $(PREFIX).dit

