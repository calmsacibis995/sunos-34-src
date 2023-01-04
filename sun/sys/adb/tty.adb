#include "../h/types.h"
#include "../h/tty.h"

tty
./"rawq.cc"16t"rawq.cf"16t"rawq.cl"n{t_rawq.c_cc,D}{t_rawq.c_cf,X}{t_rawq.c_cl,X}
+/"canq.cc"16t"canq.cf"16t"canq.cl"n{t_canq.c_cc,D}{t_canq.c_cf,X}{t_canq.c_cl,X}
+/"outq.cc"16t"outq.cf"16t"outq.cl"n{t_outq.c_cc,D}{t_outq.c_cf,X}{t_outq.c_cl,X}
+/"oproc"16t"rsel"16t"wsel"16t"addr"n{t_oproc,p}{t_rsel,X}{t_wsel,X}{t_addr,X}
+/"maj"8t"min"8t"flags"16t"state"16t"pgrp"n{t_dev,2b}{t_flags,X}{t_state,X}{t_pgrp,d}
+/"char"8t"line"8t"col"8t"ispeed"8t"ospeed"n{t_delct,b}{t_line,b}{t_col,b}{t_ispeed,b}{t_ospeed,b}
+/"rocount"8t"rocol"n{t_rocount,b}{t_rocol,b}
+/"erase"8t"kill"8t"intrc"8t"quitc"8t"startc"n{t_erase,C}8t{t_kill,C}8t{t_intrc,C}8t{t_quitc,C}8t{t_startc,C}
+/"stopc"8t"eofc"8t"brkc"8t"suspc"8t"dsuspc"n{t_stopc,C}8t{t_eofc,C}8t{t_brkc,C}8t{t_suspc,C}8t{t_dsuspc,C}
+/"rprntc"8t"flushc"8t"werase"8t"lnextc"n{t_rprntc,C}8t{t_flushc,C}8t{t_werasc,C}8t{t_lnextc,C}
+/"nlines"16t"ncols"n{t_nlines,D}{t_ncols,D}
+,<9-1$<tty
