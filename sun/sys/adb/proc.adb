#include "../h/types.h"
#include "../h/time.h"
#include "../h/proc.h"

proc
./"link"16t"rlink"16t"addr"n{p_link,X}{p_rlink,X}{p_addr,X}
+/"upri"8t"pri"8t"cpu"8t"stat"8t"time"8t"nice"8t"slp"n{p_usrpri,b}{p_pri,b}{p_cpu,b}{p_stat,b}{p_time,b}{p_nice,b}{p_slptime,b}
+/"cursig"16t"sig"n{p_cursig,b}16t{p_sig,X}
+/"mask"16t"ignore"16t"catch"n{p_sigmask,X}{p_sigignore,X}{p_sigcatch,X}
+/"flag"16t"uid"8t"pgrp"8t"pid"8t"ppid"n{p_flag,X}{p_uid,d}{p_pgrp,d}{p_pid,d}{p_ppid,d}
+/"xstat"16t"ru"16t"poip"8t"szpt"8t"tsize"n{p_xstat,x}16t{p_ru,X}{p_poip,x}{p_szpt,x}{p_tsize,X}
+/"dsize"16t"ssize"16t"rssize"16t"maxrss"n{p_dsize,X}{p_ssize,X}{p_rssize,X}{p_maxrss,X}
+/"swrss"16t"swaddr"16t"wchan"16t"textp"n{p_swrss,X}{p_swaddr,X}{p_wchan,X}{p_textp,X}
+/"p0br"16t"p1br"16t"xlink"16t"ticks"n{p_p0br,X}{p_p1br,X}{p_xlink,X}{p_cpticks,d}
+/"%cpu"16t"ndx"8t"idhash"8t"pptr"16t"tptr"n{p_pctcpu,X}{p_ndx,x}{p_idhash,x}{p_pptr,X}{p_tptr,X}
+/"real itimer"n{p_realtimer,4D}
+/"ctx"n{p_ctx,X}
+/"suid"n{p_suid,d}
+,<9-1$<proc
