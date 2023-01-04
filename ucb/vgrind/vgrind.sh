#! /bin/csh -f
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)vgrind.sh 1.1 86/09/25 SMI; from UCB 5.3 (Berkeley) 11/13/85
#
# vgrind

# Get formatter from environment, if present, using "troff" as default.
if ( $?TROFF ) then
	set troff = "$TROFF"
else
	set troff = "troff"
endif

set b=/usr/lib
set troffopts=
set options=
set files=
set f=''
set head=""
top:
if ($#argv > 0) then
    switch ($1:q)

    case -d:
	if ($#argv < 2) then
	    echo "vgrind: $1:q option must have argument"
	    goto done
	else
	    set options = ($options $1:q $2)
	    shift
	    shift
	    goto top
	endif

    case -f:
	set f='filter'
	set options = "$options $1:q"
	shift
	goto top

    case -h:
	if ($#argv < 2) then
	    echo "vgrind: $1:q option must have argument"
	    goto done
	else
	    set head="$2"
	    shift
	    shift
	    goto top
	endif

    case -o*:
	set troffopts="$troffopts $1:q"
	shift
	goto top

    case -P*:    # Printer specification -- pass on to troff for disposition
	set troffopts="$troffopts $1:q"
	shift
	goto top

    case -T*:    # Output device specification -- pass on to troff
	set troffopts="$troffopts $1:q"
	shift
	goto top

    case -t:
	set troffopts = "$troffopts -t"
	shift
	goto top

    case -W:
	set troffopts = "$troffopts -W"
	shift
	goto top

    case -w:    # Alternative tab width (4 chars) -- vfontedpr wants it as -t
	set options="$options -t"
	shift
	goto top

    case -*:
	set options = "$options $1:q"
	shift
	goto top

    default:
	set files = "$files $1:q"
	shift
	goto top
    endsw
endif

if (-r index) then
    echo > nindex
    foreach i ($files)
	#	make up a sed delete command for filenames
	#	being careful about slashes.
	echo "? $i ?d" | sed -e "s:/:\\/:g" -e "s:?:/:g" >> nindex
    end
    sed -f nindex index >xindex
    if ($f == 'filter') then
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files | cat $b/tmac/tmac.vgrind -
	else
	    $b/vfontedpr $options $files | cat $b/tmac/tmac.vgrind -
	endif
    else
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files | \
		/bin/sh -c "$troff -rx1 $troffopts -i -mvgrind 2>> xindex"
	else
	    $b/vfontedpr $options $files | \
		/bin/sh -c "$troff -rx1 $troffopts -i -mvgrind 2>> xindex"
	endif
    endif
    sort -df +0 -2 xindex >index
    rm nindex xindex
else
    if ($f == 'filter') then
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files | cat $b/tmac/tmac.vgrind -
	else
	    $b/vfontedpr $options $files | cat $b/tmac/tmac.vgrind -
	endif
    else
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files \
		| $troff -i $troffopts -mvgrind
	else
	    $b/vfontedpr $options $files \
		| $troff -i $troffopts -mvgrind
	endif
    endif
endif

done:
