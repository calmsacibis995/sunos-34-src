#include "../h/types.h"
#include "../h/dir.h"

direct
./{d_ino,X}{d_name,s}
.+({*d_reclen,.})>a
<a,<9-1$<dir.nxt
