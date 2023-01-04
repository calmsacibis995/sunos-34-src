/*
 * @(#)nlm_prot.x 1.1 86/09/25
 *
 */
/*
 * protocol used between local lock manager and remote lock manager
 */

#include <rpcsvc/klm_prot.h>
/*
 * Over-the-wire protocol used between the network lock managers
 */

program NLM_PROG {
	version NLM_VERS {

		nlm_testres	NLM_TEST(struct nlm_testargs) =	1;

		nlm_res		NLM_LOCK(struct nlm_lockargs) =	2;

		nlm_res		NLM_CANCEL(struct nlm_cancargs) = 3;
		nlm_res		NLM_UNLOCK(struct nlm_unlockargs) =	4;

		/*
		 * remote lock manager call-back to grant lock
		 */
		nlm_res		NLM_GRANTED(struct nlm_testargs)= 5;
		/*
		 * message passing style of requesting lock
		 */
		void		NLM_TEST_MSG(struct nlm_testargs) = 6;
		void		NLM_LOCK_MSG(struct nlm_lockargs) = 7;
		void		NLM_CANCEL_MSG(struct nlm_cancargs) =8;
		void		NLM_UNLOCK_MSG(struct nlm_unlockargs) = 9;
		void		NLM_GRANTED_MSG(struct nlm_testargs) = 10;
		void		NLM_TEST_RES(nlm_testres) = 11;
		void		NLM_LOCK_RES(nlm_res) = 12;
		void		NLM_CANCEL_RES(nlm_res) = 13;
		void		NLM_UNLOCK_RES(nlm_res) = 14;
		void		NLM_GRANTED_RES(nlm_res) = 15;

	} = 1;
} = 100021;

#define LM_MAXSTRLEN	1024

/*
 * status of a call to the lock manager
 */
enum nlm_stats {
	nlm_granted = 0,
	nlm_denied = 1,
	nlm_denied_nolocks = 2,
	nlm_blocked = 3,
	nlm_denied_grace_period = 4
};

struct nlm_holder {
	bool exclusive;
	int svid;
	netobj oh;
	unsigned l_offset;
	unsigned l_len;
};

union nlm_testrply switch (nlm_stats stat) {
	case nlm_granted:
		void v;		/* the lock is 'grantable' */
	case nlm_denied:
		struct nlm_holder holder;
	case nlm_denied_nolocks:
		void v;
	case nlm_blocked:
		void v;
	case nlm_denied_grace_period:
		void v;
};

struct nlm_stat {
	nlm_stats stat;
};

struct nlm_res {
	netobj cookie;
	nlm_stat stat;
};

struct nlm_testres {
	netobj cookie;
	nlm_testrply stat;
};

struct nlm_lock {
	string caller_name[LM_MAXSTRLEN];
	netobj fh;		/* identify a file */
	netobj oh;		/* identify owner of a lock */
	int svid;	/* generated from pid for svid */
	unsigned l_offset;
	unsigned l_len;
};

struct nlm_lockargs {
	netobj cookie;
	bool block;
	bool exclusive;
	struct nlm_lock lock;
	bool reclaim;		/* used for recovering locks */
	int state;		/* specify local status monitor state */
};

struct nlm_cancargs {
	netobj cookie;		
	bool block;
	bool exclusive;
	struct nlm_lock lock;
};

struct nlm_testargs {
	netobj cookie;		
	bool exclusive;
	struct nlm_lock lock;
};

struct nlm_unlockargs {
	netobj cookie;		
	struct nlm_lock lock;
};


