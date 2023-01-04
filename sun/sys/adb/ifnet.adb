#include "../h/types.h"
#include "../h/socket.h"
#include "../net/if.h"

ifnet
./"unit"8t"mtu"8t"net"16t"flags"n{if_unit,d}{if_mtu,d}{if_net,X}{if_flags,x}n"host"16t{if_host,2X}n{if_addr,3X}"addr"n{if_broadaddr,3X}"broad"n"head"16t"tail"16t"len"16t"drops"n{if_snd.ifq_head,X}{if_snd.ifq_tail,X}{if_snd.ifq_len,D}{if_snd.ifq_drops,D}n"ipack"16t"ierr"n{if_ipackets,D}{if_ierrors,D}n"opack"16t"oerr"16t"coll"n{if_opackets,D}{if_oerrors,D}{if_collisions,D}n"next"n{if_next,X}n
