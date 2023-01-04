#include "../h/types.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"

tcpcb
./"nxt"16t"prev"16t"state"n{seg_next,X}{seg_prev,X}{t_state,d}n"rexmt"8t"pers"8t"keep"8t"2msl"8t"rxtshft"n{t_timer[TCPT_REXMT],d}{t_timer[TCPT_PERSIST],d}{t_timer[TCPT_KEEP],d}{t_timer[TCPT_2MSL],d}{t_rxtshift,d}n"tcpopt"16t"ipopt"16t"maxseg"8t"force"8t"flags"n{t_tcpopt,X}{t_ipopt,X}{t_maxseg,d}{t_force,b}{t_flags,b}n"templ"16t"inpcb"16tn{t_template,X}{t_inpcb,X}n"snduna"16t"sndnxt"16t"sndup"n{snd_una,X}{snd_nxt,X}{snd_up,X}n"sndwl1"16t"sndwl2"16t"iss"16t"sndwnd"n{snd_wl1,X}{snd_wl2,X}{iss,X}{snd_wnd,d}n"rcvwnd"16t"rcvnxt"16t"rcvup"16t"irs"n{rcv_wnd,d}16t{rcv_nxt,X}{rcv_up,X}{irs,X}"rcvadv"16t"sndmax"16t"idle"8t"rtt"n{rcv_adv,X}{snd_max,X}{t_idle,d}{t_rtt,d}n"rtseq"16t"srtt"n{t_rtseq,X}{t_srtt,X}
