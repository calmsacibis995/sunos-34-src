#include "../h/types.h"
#include "../h/uio.h"

uio
./"iovcnt"16t"offset"16t"seg"16t"resid"n{uio_iovcnt,D}{uio_offset,D}{uio_seg,D}{uio_resid,D}"base"16t"len"
{*uio_iov,.},{*uio_iovcnt,.}$<iovec
