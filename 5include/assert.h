/*	@(#)assert.h 1.1 86/09/24 SMI; from S5R2 1.4	*/

#ifdef NDEBUG
#define assert(EX)
#else
extern void _assert();
#define assert(EX) if (EX) ; else _assert("EX", __FILE__, __LINE__)
#endif
