#include "../h/types.h"
#include "../h/time.h"
#include "../h/vnode.h"

vnode
./"flag"8t"refct"8t"shlockc"8t"exlockc"n{v_flag,x}{v_count,d}{v_shlockc,d}{v_exlockc,d}
+/"vfsmnt"16t"vop"16t"socket"16t"vfsp"n{v_vfsmountedhere,X}{v_op,X}{v_socket,X}{v_vfsp,X}
+/"type"16t"rdev"16t"data"n{v_type,D}{v_rdev,2b}{v_data,X}
