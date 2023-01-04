#include "../h/types.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"

icommon
./"mode"8t"links"8t"uid"8t"gid"8t"size"n{ic_mode,o}{ic_nlink,d}{ic_uid,d}{ic_gid,d}{ic_size.val[1],D}
+/"atime "{ic_atime,Y}n"mtime "{ic_mtime,Y}n"ctime "{ic_ctime,Y}
+/"addresses"
+/{ic_db,12X}{ic_ib,3X}
+/"flags"16t"blocks"16t"gen"n{ic_flags,X}{ic_blocks,D}{ic_gen,D}16+
+,<9-1$<dino
