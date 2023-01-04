#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"

user
./{u_pcb}
+$<<pcb{OFFSETOK}
+/"procp"16t"ar0"16t"comm"n{u_procp,X}{u_ar0,X}{u_comm,10C}
+/"arg0"16t"arg1"16t"arg2"n{u_arg[0],X}{u_arg[1],X}{u_arg[2],X}
+/"uap"16t"qsave"16t" "16t"error"n{u_ap,X}{u_qsave,2X}{u_error,b}
+/"rv1"16t"rv2"16t"eosys"n{u_r.r_val1,X}{u_r.r_val2,X}{u_eosys,b}
+/"u_cred"n{u_cred,X}
+/"tsize"16t"dsize"16t"ssize"n{u_tsize,X}{u_dsize,X}{u_ssize,X}
+/"odsize"16t"ossize"16t"outime"n{u_odsize,X}{u_ossize,X}{u_outime,X}
+/"signal"n{u_signal,32X}"sigmask"n{u_sigmask,32X}
+/"onstack"16t"oldmask"16t"code"n{u_sigonstack,X}{u_oldmask,X}{u_code,X}
+/"sigstack"16t"onsigstack"n{u_sigstack,X}{u_onstack,X}
+/"ofile"n{u_ofile,30X}n"pofile"n{u_pofile,30b}
+/"cdir"16t"rdir"16t"ttyp"16t"ttyd"8t"cmask"n{u_cdir,X}{u_rdir,X}{u_ttyp,X}{u_ttyd,x}{u_cmask,x}n"ru & cru"n
+,2$<<rusage{OFFSETOK}
+/"real itimer"n{u_timer[0],4D}
+/"virtual itimer"n{u_timer[1],4D}
+/"prof itimer"n{u_timer[2],4D}
