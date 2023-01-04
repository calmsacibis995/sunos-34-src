/*	@(#)search.h 1.1 86/09/24 SMI; from S5R2 1.2	*/

/* HSEARCH(3C) */
typedef struct entry { char *key, *data; } ENTRY;
typedef enum { FIND, ENTER } ACTION;

/* TSEARCH(3C) */
typedef enum { preorder, postorder, endorder, leaf } VISIT;
