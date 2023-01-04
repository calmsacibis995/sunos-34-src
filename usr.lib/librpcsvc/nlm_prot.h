/*
 * @(#)nlm_prot.h 1.1 86/09/25
 *
 * nlm_prot.h
 * generated from rpcgen -h nlm_prot.x
 */
 
#include <rpcsvc/klm_prot.h>

#define NLM_PROG 100021
#define NLM_VERS 1
#define NLM_TEST 1
#define NLM_LOCK 2
#define NLM_CANCEL 3
#define NLM_UNLOCK 4
#define NLM_GRANTED 5
#define NLM_TEST_MSG 6
#define NLM_LOCK_MSG 7
#define NLM_CANCEL_MSG 8
#define NLM_UNLOCK_MSG 9
#define NLM_GRANTED_MSG 10
#define NLM_TEST_RES 11
#define NLM_LOCK_RES 12
#define NLM_CANCEL_RES 13
#define NLM_UNLOCK_RES 14
#define NLM_GRANTED_RES 15

#define LM_MAXSTRLEN	1024

enum nlm_stats {
	nlm_granted = 0,
	nlm_denied = 1,
	nlm_denied_nolocks = 2,
	nlm_blocked = 3,
	nlm_denied_grace_period = 4,
};
typedef enum nlm_stats nlm_stats;


struct nlm_holder {
	bool_t exclusive;
	int svid;
	netobj oh;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_holder nlm_holder;


struct nlm_testrply {
	nlm_stats stat;
	union {
		struct nlm_holder holder;
	} nlm_testrply;
};
typedef struct nlm_testrply nlm_testrply;


struct nlm_stat {
	nlm_stats stat;
};
typedef struct nlm_stat nlm_stat;


struct nlm_res {
	netobj cookie;
	nlm_stat stat;
};
typedef struct nlm_res nlm_res;


struct nlm_testres {
	netobj cookie;
	nlm_testrply stat;
};
typedef struct nlm_testres nlm_testres;


struct nlm_lock {
	char *caller_name;
	netobj fh;
	netobj oh;
	int svid;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_lock nlm_lock;


struct nlm_lockargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock lock;
	bool_t reclaim;
	int state;
};
typedef struct nlm_lockargs nlm_lockargs;


struct nlm_cancargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock lock;
};
typedef struct nlm_cancargs nlm_cancargs;


struct nlm_testargs {
	netobj cookie;
	bool_t exclusive;
	struct nlm_lock lock;
};
typedef struct nlm_testargs nlm_testargs;


struct nlm_unlockargs {
	netobj cookie;
	struct nlm_lock lock;
};
typedef struct nlm_unlockargs nlm_unlockargs;

bool_t xdr_nlm_stats();
bool_t xdr_nlm_holder();
bool_t xdr_nlm_testrply();
bool_t xdr_nlm_stat();
bool_t xdr_nlm_res();
bool_t xdr_nlm_testres();
bool_t xdr_nlm_lock();
bool_t xdr_nlm_lockargs();
bool_t xdr_nlm_cancargs();
bool_t xdr_nlm_testargs();
bool_t xdr_nlm_unlockargs();
