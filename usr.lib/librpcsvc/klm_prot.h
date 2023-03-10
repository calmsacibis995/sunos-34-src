/*      @(#)klm_prot.h 1.1 86/09/25 SMI */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#define KLM_PROG 100020
#define KLM_VERS 1
#define KLM_TEST 1
#define KLM_LOCK 2
#define KLM_CANCEL 3
#define KLM_UNLOCK 4

#define LM_MAXSTRLEN	1024

enum klm_stats {
	klm_granted = 0,
	klm_denied = 1,
	klm_denied_nolocks = 2,
	klm_working = 3,
};
typedef enum klm_stats klm_stats;


struct klm_lock {
	char *server_name;
	netobj fh;
	int pid;
	u_int l_offset;
	u_int l_len;
};
typedef struct klm_lock klm_lock;


struct klm_holder {
	bool_t exclusive;
	int svid;
	u_int l_offset;
	u_int l_len;
};
typedef struct klm_holder klm_holder;


struct klm_stat {
	klm_stats stat;
};
typedef struct klm_stat klm_stat;


struct klm_testrply {
	klm_stats stat;
	union {
		struct klm_holder holder;
	} klm_testrply;
};
typedef struct klm_testrply klm_testrply;


struct klm_lockargs {
	bool_t block;
	bool_t exclusive;
	struct klm_lock lock;
};
typedef struct klm_lockargs klm_lockargs;


struct klm_testargs {
	bool_t exclusive;
	struct klm_lock lock;
};
typedef struct klm_testargs klm_testargs;


struct klm_unlockargs {
	struct klm_lock lock;
};
typedef struct klm_unlockargs klm_unlockargs;

bool_t xdr_klm_stats();
bool_t xdr_klm_lock();
bool_t xdr_klm_holder();
bool_t xdr_klm_stat();
bool_t xdr_klm_testrply();
bool_t xdr_klm_lockargs();
bool_t xdr_klm_testargs();
bool_t xdr_klm_unlockargs();
