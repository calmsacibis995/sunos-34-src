#include "../h/types.h"
#include "../h/buf.h"

buf
./"flags"n{b_flags,X}n"forw"16t"back"16t"av_forw"16t"av_back"n{b_forw,X}{b_back,X}{av_forw,X}{av_back,X}"bcount"16t"bufsize"16t"error"8t"major"8t"minor"n{b_bcount,D}{b_bufsize,D}{b_error,d}{b_dev,2b}n"addr"16t"blkno"16t"resid"16t"proc"n{b_un.b_addr,X}{b_blkno,D}{b_resid,D}{b_proc,X}
+,<9-1$<buf
