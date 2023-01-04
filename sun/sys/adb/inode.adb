#include "../h/types.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"

inode
./"forw"16t"back"n{i_chain[0],X}{i_chain[1],X}
+$<<vnode{OFFSETOK}
+/"devvp"16t"flag"8t"maj"8t"min"8t"ino"n{i_devvp,X}{i_flag,x}{i_dev,2b}{i_number,D}
+/"fs"16t"dquot"n{i_fs,X}{i_dquot,X}
+/"lastr"16t"freef"16t"freeb"n{i_un.if_lastr,D}{i_fr.if_freef,X}{i_fr.if_freeb,X}
+$<<dino
.,<9-1$<inode
