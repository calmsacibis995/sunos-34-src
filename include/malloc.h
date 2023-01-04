/*	@(#)malloc.h 1.1 86/09/24 SMI; from S5R2 1.2	*/

/*
	Constants defining mallopt operations
*/
#define M_MXFAST	1	/* set size of 'small blocks' */
#define M_NLBLKS	2	/* set num of small blocks in holding block */
#define M_GRAIN		3	/* set rounding factor for small blocks */
#define M_KEEP		4	/* (nop) retain contents of freed blocks */

/*
	malloc information structure
*/
struct mallinfo  {
	int arena;	/* total space in arena */
	int ordblks;	/* number of ordinary blocks */
	int smblks;	/* number of small blocks */
	int hblks;	/* number of holding blocks */
	int hblkhd;	/* space in holding block headers */
	int usmblks;	/* space in small blocks in use */
	int fsmblks;	/* space in free small blocks */
	int uordblks;	/* space in ordinary blocks in use */
	int fordblks;	/* space in free ordinary blocks */
	int keepcost;	/* cost of enabling keep option */

	int mxfast;	/* max size of small blocks */
	int nlblks;	/* number of small blocks in a holding block */
	int grain;	/* small block rounding factor */
	int uordbytes;	/* space (including overhead) allocated in ord. blks */
	int allocated;	/* number of ordinary blocks allocated */
	int treeoverhead;	/* bytes used in maintaining the free tree */
};	

extern	char	*malloc();
extern	char	*realloc();
extern	int	mallopt();
extern	struct mallinfo mallinfo();
