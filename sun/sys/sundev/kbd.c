#ifndef lint
static	char sccsid[] = "@(#)kbd.c 1.5 87/02/12 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (C) 1986 by Sun Microsystems, Inc.
 */
/*
 * Keyboard input line discipline
 * KBDLDISC handles conversion of up/down codes to ASCII
 */
#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/conf.h"
#include "../h/systm.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../h/reboot.h"
#include "../mon/sunromvec.h"
#include "../sun/consdev.h"
#include "../sundev/kbd.h"
#include "../sundev/kbio.h"
#include "../sundev/kbdreg.h"
#include "../sundev/vuid_event.h"
#include "../sundev/vuid_queue.h"
#include "../debug/debug.h"

/*
 * For now these are shared.
 */
extern int nkeytables;
extern struct keyboard	*keytables[];
extern char keystringtab[16][KTAB_STRLEN];

/*
 * Keyboard instance data.
 */
#define	NKBDS		4
int	kbd_downs_size = 15;	/* This value corresponds approximately to
				   max 10 fingers */
int	kbd_vq_nodes = 40;	/* Number of keyboard event can queue up */
typedef	struct	key_event {
	u_char	key_station;	/* Physical key station associated with event */
	Firm_event event;	/* Event that sent out on down */
} Key_event;
struct	kbddata {
	struct	keyboardstate kbdd_state;
					/* State of keyboard & keyboard
					   specific settings, e.g., tables */
	int	kbdd_translate;		/* Translate keycodes? */
	int	kbdd_translatable;	/* Keyboard is translatable? */
	struct	tty *kbdd_tp;		/* Tty connected to */
	int	kbdd_directio;		/* Keyboard device is open?
					   If not, route keystrokes on
					   kbddev to cninput */
	int	(*kbdd_usecode)();	/* XXX - call out which calls back
					   for translation
					   SHOULD GO AWAY IN 4.0 */
	short	kbdd_ascii_addr;	/* Vuid_id_addr for ascii events */
	short	kbdd_top_addr;		/* Vuid_id_addr for top events */
	short	kbdd_vkey_addr;		/* Vuid_id_addr for vkey events */
	short	kbdd_sg_flags;		/* Tty mode flags */
	Vuid_queue kbdd_q;		/* kbd input q */
	caddr_t	kbdd_qdata;		/* address of kbdd_q data block */
	u_int	kbdd_qbytes;		/* # of bytes used for kbdd_qdata */
	int	kbdd_qnodes;		/* # of nodes allocated for kbdd_q */
	struct	key_event *kbdd_downs;	/* Table of key stations currently down
					   that have firm events that need
					   to be matched with up transitions
					   when kbdd_translate is TR_*EVENT */
	int	kbdd_downs_entries;	/* # of possible entries in kbdd_downs*/
	u_int	kbdd_downs_bytes;	/* # of bytes allocated for kbdd_downs*/
};
struct	kbddata kbddata[NKBDS];
static struct kbddata *kbdtptokbdd();
static void kbd_send_esc_event();

/*
 * Constants setup during the first open of a kbd (so that hz is defined).
 */
int	kbd_repeatrate;
int	kbd_repeatdelay;

int	kbd_overflow_cnt;	/* Number of times kbd overflowed input q */
int	kbd_overflow_msg = 1;	/* Whether to print message on q overflow */

#ifdef	KBD_DEBUG
int	kbd_debug = 0;
int	kbd_ra_debug = 0;
int	kbd_raw_debug = 0;
int	kbd_rpt_debug = 0;
int	kbd_input_debug = 0;
#endif	KBD_DEBUG

/*
 * Open a keyboard.
 * Ttyopen sets line characteristics
 */
/* ARGSUSED */
kbdopen(dev, tp)
	dev_t dev;
	struct	tty *tp;
{
	register int err, i;
	struct	sgttyb sg;
        register struct cdevsw *dp;
	register struct	kbddata *kbdd;

	/* Set these up only once so that they could be changed from adb */
	if (!kbd_repeatrate) {
		kbd_repeatrate = (hz+29)/30;
		kbd_repeatdelay = hz/2;
	}
	/*
	 * See if tp is being used to drive kbd already.
	 */
	for (i = 0;i < NKBDS; ++i)
		if (kbddata[i].kbdd_tp == tp)
			return (0);
	/*
	 * Get next free kbddata.
	 */
	for (i = 0;i < NKBDS; ++i)
		if (kbddata[i].kbdd_tp == 0) {
			kbdd = &kbddata[i];
			goto found;
		}
	return (EBUSY);
found:
	/*
	 * Open tty.
	 */
	if (err = ttyopen(dev, tp))
		return (err);
	/*
	 * Setup tty flags
	 */
        dp = &cdevsw[major(dev)];
        if (err = (*dp->d_ioctl) (dev, TIOCGETP, (caddr_t)&sg, 0))
		goto error;
	sg.sg_flags = RAW+ANYP;
	sg.sg_ispeed = sg.sg_ospeed = B1200;
        if (err = (*dp->d_ioctl) (dev, TIOCSETP, (caddr_t)&sg, 0))
		goto error;
	/*
	 * Set up private data.
	 */
	kbdd->kbdd_usecode = NULL;	/* XXX */
	kbdd->kbdd_directio = 0;
	kbdd->kbdd_tp = tp;
	kbdd->kbdd_translatable = TR_CAN;
	kbdd->kbdd_translate = TR_ASCII;
	kbdd->kbdd_ascii_addr = ASCII_FIRST;
	kbdd->kbdd_top_addr = TOP_FIRST;
	kbdd->kbdd_vkey_addr = VKEY_FIRST;
	/* Allocate dynamic memory for q and downs table */
	kbdd->kbdd_qnodes = kbd_vq_nodes;
	kbdd->kbdd_qbytes = kbd_vq_nodes * sizeof (Vuid_q_node);
	kbdd->kbdd_qdata = (caddr_t) kmem_alloc(kbdd->kbdd_qbytes);
	if (kbdd->kbdd_qdata == NULL) {
		printf("kbd: Couldn't allocate dynamic memory for qdata\n");
		err = ENOMEM;
		goto error;
	}
	kbdd->kbdd_downs_entries = kbd_downs_size;
	kbdd->kbdd_downs_bytes = kbd_downs_size * sizeof (Key_event);
	kbdd->kbdd_downs = (Key_event *) kmem_alloc(kbdd->kbdd_downs_bytes);
	if (kbdd->kbdd_downs == NULL) {
		printf("kbd: Couldn't allocate dynamic memory for downs table\n");
		kmem_free(kbdd->kbdd_qdata, kbdd->kbdd_qbytes);
		err = ENOMEM;
		goto error;
	}
	vq_initialize(&kbdd->kbdd_q, kbdd->kbdd_qdata, kbdd->kbdd_qbytes);
	/*
	 * Reset kbd.
	 */
	kbdreset(tp);
	return (0);
error:
	bzero((caddr_t)kbdd, sizeof(struct kbddata));
	return (err);
}

/*
 * Close a keyboard.
 */
kbdclose(tp)
	struct	tty *tp;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);

	if (kbdd == 0)
		return;
	/*
	 * The program which opened the keyboard device is done with it.
	 * We clear kbdd_directio so that incoming keystrokes get directed
	 * to /dev/console if this is kbddev.
	 */
	kbdd->kbdd_directio = 0;
	/*
	 * Don't close kbddev.  Otherwise can't get to monitor.
	 * Kbddev is "opened" in sun/autoconf.c but a reference count
	 * for the device is never incremented.  This explains why
	 * close is called at all while it is still being used.
	 */
	if (tp == (cdevsw[major(kbddev)].d_ttys + minor(kbddev)))
		return;
	/*
	 * Close tty then zero kbddata structure.
	 */
	kmem_free(kbdd->kbdd_qdata, kbdd->kbdd_qbytes);
	kmem_free((caddr_t)kbdd->kbdd_downs, kbdd->kbdd_downs_bytes);
	ttyclose(tp);
	bzero((caddr_t)kbdd, sizeof (*kbdd));
}

/*
 * Keyboard ioctl - only supports KIOC*
 */
kbdioctl(tp, cmd, data, flag)
	struct	tty *tp;
	int cmd;
	register caddr_t data;
	int flag;
{
	register int err;
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register short	new_translate;
	register Vuid_addr_probe *addr_probe;
	register short	*addr_ptr;
	int	pri;

	if (kbdd == 0)
		return (EINVAL);
	err = 0;
	pri = spl5();
	switch (cmd) {

	case FIONREAD:
		switch (kbdd->kbdd_translate) {
		case TR_EVENT:
		case TR_UNTRANS_EVENT:
			*(int *)data =
			    sizeof (Firm_event) * vq_used(&kbdd->kbdd_q);
			break;

		case TR_ASCII:
		case TR_NONE:
			*(int *)data = tp->t_rawq.c_cc;
			break;
		}
		break;

	case VUIDSFORMAT:
		new_translate = (*(int *)data == VUID_NATIVE)? TR_ASCII:
		    TR_EVENT;
		if (new_translate == kbdd->kbdd_translate)
			break;
		kbdd->kbdd_translate = new_translate;
		goto output_format_change;

	case KIOCTRANS:
		new_translate = *(int *)data;

		if (new_translate == kbdd->kbdd_translate)
			break;
		kbdd->kbdd_translate = new_translate;
		goto output_format_change;

	case KIOCCMD:
		kbdcmd(tp, (char)(*(int *)data));
		break;

	case VUIDGFORMAT:
		*(int *)data = (kbdd->kbdd_translate == TR_EVENT ||
				kbdd->kbdd_translate == TR_UNTRANS_EVENT) ?
			     VUID_FIRM_EVENT: VUID_NATIVE;
		break;

	case KIOCGTRANS:
		*(int *)data = kbdd->kbdd_translate;
		break;

	case VUIDSADDR:
		addr_probe = (Vuid_addr_probe *)data;
		switch (addr_probe->base) {

		case ASCII_FIRST:
			addr_ptr = &kbdd->kbdd_ascii_addr;
			break;

		case TOP_FIRST:
			addr_ptr = &kbdd->kbdd_top_addr;
			break;

		case VKEY_FIRST:
			addr_ptr = &kbdd->kbdd_vkey_addr;
			break;

		default:
			err = ENODEV;
		}
		if ((err == 0) && (*addr_ptr != addr_probe->data.next)) {
			*addr_ptr = addr_probe->data.next;
			goto output_format_change;
		}
		break;

	case VUIDGADDR:
		addr_probe = (Vuid_addr_probe *)data;
		switch (addr_probe->base) {

		case ASCII_FIRST:
			addr_probe->data.current = kbdd->kbdd_ascii_addr;
			break;

		case TOP_FIRST:
			addr_probe->data.current = kbdd->kbdd_top_addr;
			break;

		case VKEY_FIRST:
			addr_probe->data.current = kbdd->kbdd_vkey_addr;
			break;

		default:
			err = ENODEV;
		}
		break;

	case KIOCTRANSABLE:
		if (kbdd->kbdd_translatable != *(int *)data) {
			kbdd->kbdd_translatable = *(int *)data;
			goto output_format_change;
		}
		break;

	case KIOCGTRANSABLE:
		*(int *)data = kbdd->kbdd_translatable;
		break;

	case KIOCSETKEY:
		err = kbdsetkey(tp, (struct kiockey *)data);
		/*
		 * Since this only affects any subsequent key presses,
		 * don't goto output_format_change.  One might want to
		 * toggle the keytable entries dynamically.
		 */
		break;

	case KIOCGETKEY:
		err = kbdgetkey(tp, (struct kiockey *)data);
		break;

	case KIOCSDIRECT:
		kbdd->kbdd_directio = *(int *)data;
		goto output_format_change;

	case KIOCGDIRECT:
		*(int *)data = kbdd->kbdd_directio;
		break;

	/*
	 * The next two "ioctl"s are only for backward compatibility
	 * with drivers written before 3.2.  They should go away in 4.0.
	 */
	case KIOCSUSECODE:
		/*
		 * This intricate cast is used in order to squelch
		 * "lint"s (completely justified!) complaints about
		 * questionable casting of function pointers.  At some
		 * point this "ioctl" will go away and the problem
		 * will go away with it.
		 */
		kbdd->kbdd_usecode = (int (*)())(int)data;
		goto output_format_change;

	case KIOCGUSECODE:
		*(int *)data = (int)kbdd->kbdd_usecode;
		break;

	case KIOCTYPE:
		*(int *)data = (kbdd->kbdd_state.k_idstate == KID_OK)?
		    kbdd->kbdd_state.k_id: -1;
		break;

	case TIOCSETD:
		/*
		 * Don't let the line discipline change once it has been set
		 * to a keyboard.  Changing the ldisc causes kbdclose to be
		 * called even if the ldisc of the tp is the same.
		 * We can't let this happen because kbdclose will reset
		 * the keyboard away from direct I/O mode.
		 * The basic problem is that the "console keyboard" (and
		 * "console mouse") indirect drivers set the line discipline
		 * of the device they open.  I'm not sure why this is done,
		 * since if you open any other device (zs or pi) directly
		 * for use as a keyboard or mouse device, you have to
		 * set the line discipline, so any program which can open
		 * general devices has to set the line discipline anyway.
		 * Programs which only open /dev/kbd or /dev/mouse will
		 * still have to set the keyboard to direct I/O mode,
		 * so there's no added convenience of having the kernel
		 * set the line discipline for you.
		 */
		if (*(int *)data == KBDLDISC)
			break;

	default:
		(void) splx(pri);
		return (ttioctl(tp, cmd, data, flag));
	}
	(void) splx(pri);
	return (err);

output_format_change:
	kbdflush(tp);
	kbdreset(tp);
	(void) splx(pri);
	return (err);
}
 
kbdflush(tp)
	struct  tty *tp;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	Firm_event fe;

	/* Flush pending data already sent to tty */
	ttyflush(tp, FREAD);
	/* Flush unread input */
	while (vq_get(&kbdd->kbdd_q, &fe) != VUID_Q_EMPTY) {}
	/* Flush pending ups */
	bzero((caddr_t)(kbdd->kbdd_downs), kbdd->kbdd_downs_bytes);
	kbdcancelrpt(tp);
}

/*
 * Set/Reset translation of keycodes (called from outside kbd.c)
 */
kbdsettrans(tp, translate)
        struct  tty *tp;
        int     translate;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);

	if (kbdd == 0)
		return (EINVAL);
	kbdd->kbdd_translate = translate;
	return (0);
}
 
/*
 * Call consumer of keycode.
 */
kbduse(tp, keycode, k)
	struct	tty *tp;
	u_char	keycode;
	register struct keyboardstate *k;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	extern int cninput();
	extern int ttyinput();

#ifdef	KBD_DEBUG
	if (kbd_input_debug) printf("KBD USE key=%d\n", keycode);
#endif

	if (kbdd == 0)
		return;
	/*
	 * XXX - if "kbdd_usecode" is set, use it.
	 */
	if (kbdd->kbdd_usecode != 0) {
		(*(kbdd->kbdd_usecode))(tp, keycode, k);
		return;
	}
	if (!kbdd->kbdd_directio) {
		/*
		 * If keyboard isn't open, throw away keycodes unless
		 * this is "kbddev"; if it is, pass them up to /dev/console.
		 */
		if (tp == cdevsw[major(kbddev)].d_ttys + minor(kbddev)) {
			if (!kbdd->kbdd_translatable
			    || kbdd->kbdd_translate == TR_NONE)
				cninput((char)keycode, tp);
			else
				kbdtranslate(tp, keycode, cninput);
		}
	} else {
		if (!kbdd->kbdd_translatable
		    || kbdd->kbdd_translate == TR_NONE)
			ttyinput((char)keycode, tp);
		else
			kbdtranslate(tp, keycode, ttyinput);
	}
}

/*
 * Match *tp to kbddata.
 */
static struct kbddata *
kbdtptokbdd(tp)
	struct	tty **tp;
{
	register i;

	/*
	 * If talking about the console then use kbddev.
	 */
	if (*tp == cdevsw[0/*console*/].d_ttys)
		*tp = cdevsw[major(kbddev)].d_ttys + minor(kbddev);
	/*
	 * Get kbddata whose tp matches tp.
	 */
	for (i = 0; i < NKBDS; i++)
		if (kbddata[i].kbdd_tp == *tp)
			return (&kbddata[i]);
	printf("kbd: kbdtptokbdd called with unknown tp %X\n", *tp);
	return (0);
}

/*
 * kbdclick is used to remember the current click value of the
 * Sun-3 keyboard.  This brain damaged keyboard will reset the
 * clicking to the "default" value after a reset command and
 * there is no way to read out the current click value.  We
 * cannot send a click command immediately after the reset
 * command or the keyboard gets screwed up.  So we wait until
 * we get the ID byte before we send back the click command.
 * Unfortunately, this means that there is a small window
 * where the keyboard can click when it really shouldn't be.
 * A value of -1 means that kbdclick has not been initialized yet.
 */
int kbdclick = -1;

/*
 * Send command byte to keyboard
 */
kbdcmd(tp, cmd)
	struct	tty *tp;
	char cmd;
{
	int s;

	s = spl5();
	(void) ttyoutput(cmd, tp);
	ttstart(tp);
	(void) splx(s);
	if (cmd == KBD_CMD_NOCLICK)
		kbdclick = 0;
	else if (cmd == KBD_CMD_CLICK)
		kbdclick = 1;
}

/*
 * Reset the keyboard
 */
kbdreset(tp)
	struct	tty *tp;
{
	register struct keyboardstate *k;
	register struct kbddata *kbdd = kbdtptokbdd(&tp);

	if (kbdd == 0)
		return;
	k = &kbdd->kbdd_state;
	if (kbdd->kbdd_translatable) {
		k->k_idstate = KID_NONE;
		k->k_state = NORMAL;
		kbdcmd(tp, KBD_CMD_RESET);
	} else {
		bzero((caddr_t)k, sizeof(struct keyboardstate));
		k->k_id = KB_ASCII;
		k->k_idstate = KID_OK;
	}
}

/*
 * Set individual keystation translation
 * TODO: Have each keyboard own own translation tables.
 */
kbdsetkey(tp, key)
	struct	tty *tp;
	struct	kiockey *key;
{
	int	strtabindex, i;
	struct	keymap *settable();
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	struct	keymap *km;

	if (kbdd == 0 || key->kio_station > 127)
		return (EINVAL);
	if (kbdd->kbdd_state.k_curkeyboard == NULL)
		return (EINVAL);
	if (key->kio_tablemask == KIOCABORT1) {
		kbdd->kbdd_state.k_curkeyboard->k_abort1 = key->kio_station;
		return (0);
	}
	if (key->kio_tablemask == KIOCABORT2) {
		kbdd->kbdd_state.k_curkeyboard->k_abort2 = key->kio_station;
		return (0);
	}
	if ((km = settable(tp, (u_int)key->kio_tablemask)) == NULL)
		return (EINVAL);
	if (key->kio_entry >= STRING && key->kio_entry <= STRING+15) {
		strtabindex = key->kio_entry-STRING;
		for (i = 0;i < KTAB_STRLEN;i++)
			keystringtab[strtabindex][i] = key->kio_string[i];
		keystringtab[strtabindex][KTAB_STRLEN-1] = '\0';
	}
	km->keymap[key->kio_station] = key->kio_entry;
	return (0);
}

/*
 * Get individual keystation translation
 */
kbdgetkey(tp, key)
	struct	tty *tp;
	struct	kiockey *key;
{
	int	strtabindex, i;
	struct	keymap *settable();
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	struct	keymap *km;

	if (kbdd == 0 || key->kio_station > 127)
		return (EINVAL);
	if (kbdd->kbdd_state.k_curkeyboard == NULL)
		return (EINVAL);
	if (key->kio_tablemask == KIOCABORT1) {
		key->kio_station = kbdd->kbdd_state.k_curkeyboard->k_abort1;
		return (0);
	}
	if (key->kio_tablemask == KIOCABORT2) {
		key->kio_station = kbdd->kbdd_state.k_curkeyboard->k_abort2;
		return (0);
	}
	if ((km = settable(tp, (u_int)key->kio_tablemask)) == NULL)
		return (EINVAL);
	key->kio_entry = km->keymap[key->kio_station];
	if (key->kio_entry >= STRING && key->kio_entry <= STRING+15) {
		strtabindex = key->kio_entry-STRING;
		for (i = 0; i < KTAB_STRLEN; i++)
			key->kio_string[i] = keystringtab[strtabindex][i];
	}
	return (0);
}

kbdidletimeout(tp)
        struct tty *tp;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct keyboardstate *k;

	untimeout(kbdidletimeout, (caddr_t)tp);
	if (kbdd == 0)
		return;
	/*
	 * Double check that was waiting for idle timeout.
	 */
	k = &kbdd->kbdd_state;
	if (k->k_idstate == KID_IDLE)
		kbdinput(IDLEKEY, tp);
}

/*
 * A keypress was received (from a parallel or serial keyboard).
 * Process it through the state machine to check for aborts
 * (l_rint for KBDLDISC)
 */
/*ARGSUSED*/
kbdinput(key, tp)
	register u_char key;
	struct tty *tp;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct keyboardstate *k;

	if (kbdd == 0)
		return;
	k = &kbdd->kbdd_state;
#ifdef	KBD_DEBUG
if (kbd_input_debug) printf("kbdinput key %X\n", key);
#endif
	switch (k->k_idstate) {

	case KID_NONE:
		if (key == IDLEKEY) {
			k->k_idstate = KID_IDLE;
			timeout(kbdidletimeout, (caddr_t)tp, hz/10);
		} else if (key == RESETKEY)
			k->k_idstate = KID_LKSUN2;
		return;

	case KID_IDLE:
		if (key == IDLEKEY)
			kbdid(tp, KB_KLUNK);
		else if (key == RESETKEY)
			k->k_idstate = KID_LKSUN2;
		else if (key & 0x80)
			kbdid(tp, (int)(KB_VT100 | (key&0x40)));
		else
			kbdreset(tp);
		return;

	case KID_LKSUN2:
		if (key == 0x02) {	    /* Sun-2 keyboard */
			kbdid(tp, KB_SUN2);
			return;
		}
		if (key == 0x03) {	    /* Sun-3 keyboard */
			kbdid(tp, KB_SUN3);
			/*
			 * We just did a reset command to a Sun-3 keyboard
			 * which sets the click back to the default
			 * (which is currently ON!).  We use the kbdclick
			 * variable to see if the keyboard should be
			 * turned on or off.  If it has not been set,
			 * then on a sun3 we use the eeprom to determine
			 * if the default value is on or off.  In the
			 * sun2 case, we default to off.
			 */
			switch (kbdclick) {
			case 0:
				kbdcmd(tp, KBD_CMD_NOCLICK);
				break;
			case 1:
				kbdcmd(tp, KBD_CMD_CLICK);
				break;
			case -1:
			default:
				{
#ifdef sun3
#include "../sun3/eeprom.h"

				if (EEPROM->ee_diag.eed_keyclick ==
				    EED_KEYCLICK)
					kbdcmd(tp, KBD_CMD_CLICK);
				else
#endif sun3
					kbdcmd(tp, KBD_CMD_NOCLICK);
				}
				break;
			}
			return;
		}
		kbdreset(tp);
		return;

	case KID_OK:
		if (key == 0 || key == 0xFF) {
			kbdreset(tp);
			return;
		}
		break;
	}
			
	switch (k->k_state) {

	normalstate:
		k->k_state = NORMAL;
	case NORMAL:
		if (k->k_curkeyboard && key == k->k_curkeyboard->k_abort1) {
			k->k_state = ABORT1;
			break;
		}
		kbduse(tp, key, k);
		if (key == IDLEKEY)
			k->k_state = IDLE1;
		break;

	case IDLE1:
		if (key & 0x80)	{	/* ID byte */
			if (k->k_id == KB_VT100)
				k->k_state = IDLE2;
			else 
				kbdreset(tp);
			break;
		}
		if (key != IDLEKEY) 
			goto normalstate;	/* real data */
		break;

	case IDLE2:
		if (key == IDLEKEY) k->k_state = IDLE1;
		else goto normalstate;
		break;

	case ABORT1:
		if (k->k_curkeyboard) {
			if (key == k->k_curkeyboard->k_abort2) {
				DELAY(100000);
				if (boothowto & RB_DEBUG) {
					CALL_DEBUG();
				} else {
					montrap(*romp->v_abortent);
				}
				k->k_state = NORMAL;
				kbduse(tp, (u_char)IDLEKEY, k);	/* fake */
				return;
			} else {
				kbduse(tp, k->k_curkeyboard->k_abort1, k);
				goto normalstate;
			}
		}
	}
}

kbdid(tp, id)
	struct	tty *tp;
	int	id;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct keyboardstate *k;

	if (kbdd == 0)
		return;
	k = &kbdd->kbdd_state;

	k->k_id = id & 0xF;
	k->k_idstate = KID_OK;
	k->k_shiftmask = 0;
	if (id & 0x40)
		/* Not a transition so don't send event */
		k->k_shiftmask |= CAPSMASK;
	k->k_buckybits = 0;
	k->k_curkeyboard = keytables[k->k_id];
	k->k_rptkey = IDLEKEY;	/* Nothing happening now */
}

/*
 * This routine determines which table we should look in to decode
 * the current keycode.
 */
struct keymap *
settable(tp, mask)
	struct	tty *tp;
	register u_int mask;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct keyboard *kp;

	if (kbdd == 0)
		return (NULL);
	kp = kbdd->kbdd_state.k_curkeyboard;
	if (kp == NULL)
		return (NULL);
	if (mask & UPMASK)
		return (kp->k_up);
	if (mask & CTRLMASK)
		return (kp->k_control);
	if (mask & SHIFTMASK)
		return (kp->k_shifted);
	if (mask & CAPSMASK)
		return (kp->k_caps);
	return (kp->k_normal); 
}

kbdrpt(tp)
	struct	tty *tp;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct keyboardstate *k;
	int	pri;

	if (kbdd == 0)
		return;
	k = &kbdd->kbdd_state;
#ifdef	KBD_DEBUG
if (kbd_rpt_debug) printf("kbdrpt key %X\n", k->k_rptkey);
#endif
	/*
	 * Since timeout is at low priority (interruptable),
	 * protect code with spl's.
	 */
	pri = spl5();
	kbdkeyreleased(tp, KEYOF(k->k_rptkey));
	kbduse(tp, k->k_rptkey, k);
	if (k->k_rptkey != IDLEKEY)
		timeout(kbdrpt, (caddr_t)tp, kbd_repeatrate);
	(void) splx(pri);
}

kbdcancelrpt(tp)
	struct	tty *tp;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct keyboardstate *k;

	if (kbdd == 0)
		return;
	k = &kbdd->kbdd_state;
	if (k->k_rptkey != IDLEKEY) {
		untimeout(kbdrpt, (caddr_t)tp);	/* Ignored if not found */
		k->k_rptkey = IDLEKEY;
	}
}

kbdtranslate(tp, keycode, rint)
	struct	tty *tp;
	register u_char keycode;
	int	(*rint)();
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register u_char key, newstate, entry;
	register u_char enF0;
	register char *cp;
	register struct keyboardstate *k;
	struct keymap *km;
	Firm_event fe;

	k = &kbdd->kbdd_state;
	newstate = STATEOF(keycode);
	key = KEYOF(keycode);

#ifdef	KBD_DEBUG
	if (kbd_input_debug) printf("KBD TRANSLATE key=%d\n", keycode);
#endif

	if (kbdd->kbdd_translate == TR_UNTRANS_EVENT) {
		if (newstate == PRESSED) {
			bzero((caddr_t) & fe, sizeof fe);
			fe.id = key;
			fe.value = 1;
			kbdqueuepress(tp, key, &fe);
		} else {
			kbdkeyreleased(tp, key);
		}
		return;
	}
	km = settable(tp, (u_int)(k->k_shiftmask | newstate));
	if (km == NULL) {		/* gross error */
		kbdcancelrpt(tp);
		return;
	}
	entry = km->keymap[key];
	enF0 = entry & 0xF0;
	/*
	 * Handle the state of toggle shifts specially.
	 * Toggle shifts should only come on downs.
	 */
	if (((entry >> 4) == (SHIFTKEYS >> 4)) &&
	    ((1 << (entry & 0x0F)) & k->k_curkeyboard->k_toggleshifts)) {
		if ((1 << (entry & 0x0F)) & k->k_togglemask) {
			newstate = RELEASED;
		} else {
			newstate = PRESSED;
		}
	}

/* We might need to add the RESET/IDLE/ERROR entries here.  FIXME? */
	if (newstate == PRESSED && entry != NOSCROLL &&
	    enF0 != SHIFTKEYS && enF0 != BUCKYBITS &&
	    !(entry >= LEFTFUNC && entry <= BOTTOMFUNC+15 &&
	    kbdd->kbdd_translate == TR_EVENT)) {
		if (k->k_rptkey != keycode) {
			kbdcancelrpt(tp);
			timeout(kbdrpt, (caddr_t)tp, kbd_repeatdelay);
			k->k_rptkey = keycode;
		}
	} else if (key == KEYOF(k->k_rptkey))		/* key going up */
		kbdcancelrpt(tp);
	if ((newstate == RELEASED) && (kbdd->kbdd_translate == TR_EVENT))
		kbdkeyreleased(tp, key);

	switch (entry >> 4) {

	case 0: case 1: case 2: case 3:
	case 4: case 5: case 6: case 7:
		switch (kbdd->kbdd_translate) {

		case TR_EVENT:
			fe.id = entry | k->k_buckybits;
			fe.value = 1;
			/* Assume "up" table only generates shift changes */
			kbdkeypressed(tp, key, &fe);
			break;

		case TR_ASCII:
			rint(entry | k->k_buckybits, tp);
			break;
		}
		break;

	case SHIFTKEYS >> 4: {
		u_int shiftbit = 1 << (entry & 0x0F);

		/* Modify toggle state (see toggle processing above) */
		if (shiftbit & k->k_curkeyboard->k_toggleshifts) {
			if (newstate == RELEASED) {
				k->k_togglemask &= ~shiftbit;
				k->k_shiftmask &= ~shiftbit;
			} else {
				k->k_togglemask |= shiftbit;
				k->k_shiftmask |= shiftbit;
			}
		} else
			k->k_shiftmask ^= shiftbit;
		if (kbdd->kbdd_translate == TR_EVENT && newstate == PRESSED){
			/*
			 * Relying on ordinal correspondence between
			 * vuid_event.h SHIFT_CAPSLOCK-SHIFT_RIGHTCTRL &
			 * kbd.h CAPSLOCK-RIGHTCTRL in order to
			 * correctly translate entry into fe.id.
			 */
			fe.id = SHIFT_CAPSLOCK + (entry & 0x0F);
			fe.value = 1;
			kbdkeypressed(tp, key, &fe);
		}
		break;
		}

	case BUCKYBITS >> 4:
		k->k_buckybits ^= 1 << (7 + (entry & 0x0F));
		if (kbdd->kbdd_translate == TR_EVENT && newstate == PRESSED){
			/*
			 * Relying on ordinal correspondence between
			 * vuid_event.h SHIFT_META-SHIFT_TOP &
			 * kbd.h METABIT-SYSTEMBIT in order to
			 * correctly translate entry into fe.id.
			 */
			fe.id = SHIFT_META + (entry & 0x0F);
			fe.value = 1;
			kbdkeypressed(tp, key, &fe);
		}
		break;

	case FUNNY >> 4:
		switch (entry) {
		case NOP:
			break;

		/*
		 * NOSCROLL/CTRLS/CTRLQ exist so that these keys, on keyboards
		 * with NOSCROLL, interact smoothly.  If a user changes
		 * his tty output control keys to be something other than those
		 * in keytables for CTRLS & CTRLQ then he effectively disables
		 * his NOSCROLL key.  However, 1) this is also what happens
		 * on a VT100, 2) users should't change their stop and start
		 * characters anyway, and 3) there's nothing we can do about
		 * it.
		 */
		case NOSCROLL:
			if (k->k_shiftmask & CTLSMASK)	goto sendcq;
			else				goto sendcs;

		case CTRLS:
		sendcs:
			/* A CTLSMASK change is not a vuid transition */
			k->k_shiftmask |= CTLSMASK;
			switch (kbdd->kbdd_translate) {

			case TR_EVENT:
				fe.id = ('S'-0x40) | k->k_buckybits;
				fe.value = 1;
				kbdkeypressed(tp, key, &fe);
				break;

			case TR_ASCII:
				rint(('S'-0x40) | k->k_buckybits, tp);
				break;
			}
			break;

		case CTRLQ:
		sendcq:
			/* A CTLSMASK change is not a vuid transition */
			switch (kbdd->kbdd_translate) {

			case TR_EVENT:
				fe.id = ('Q'-0x40) | k->k_buckybits;
				fe.value = 1;
				kbdkeypressed(tp, key, &fe);
				break;

			case TR_ASCII:
				rint(('Q'-0x40) | k->k_buckybits, tp);
				break;
			}
			k->k_shiftmask &= ~CTLSMASK;
			break;

		case IDLE:
			/*
			 * Minor hack to prevent keyboards unplugged
			 * in caps lock from retaining their capslock
			 * state when replugged.  This should be
			 * solved by using the capslock info in the 
			 * KBDID byte.
			 */
			if (keycode == NOTPRESENT)
				k->k_shiftmask = 0;
			/* Fall thru into RESET code */

		case RESET:
		gotreset:
			k->k_shiftmask &= k->k_curkeyboard->k_idleshifts;
			k->k_shiftmask |= k->k_togglemask;
			k->k_buckybits &= k->k_curkeyboard->k_idlebuckys;
			kbdcancelrpt(tp);
			kbdreleaseall(tp);
			break;

		case ERROR:
			printf("kbd: Error detected\n");
			goto gotreset;

		/*
		 * Remember when adding new entries that,
		 * if they should NOT auto-repeat,
		 * they should be put into the IF statement
		 * just above this switch block.
		 */
		default:
			goto badentry;
		}
		break;

	case STRING >> 4:
		cp = &keystringtab[entry & 0x0F][0];
		while (*cp != '\0') {
			switch (kbdd->kbdd_translate) {

			case TR_EVENT:
				kbd_send_esc_event(*cp, tp);
				break;

			case TR_ASCII:
				rint(*cp, tp);
				break;
			}
			cp++;
		}
		break;

	/*
	 * Remember when adding new entries that,
	 * if they should NOT auto-repeat,
	 * they should be put into the IF statement
	 * just above this switch block.
	 */
	default:
		if (entry >= LEFTFUNC && entry <= BOTTOMFUNC+15) {
			char	buf[10], *strsetwithdecimal();

			switch (kbdd->kbdd_translate) {

			case TR_ASCII: {
				if (newstate == RELEASED)
					break;
				cp = strsetwithdecimal(&buf[0], (u_int)entry,
				    sizeof (buf) - 1);
				rint('\033', tp); /* Escape */
				rint('[', tp);
				while (*cp != '\0') {
					rint(*cp, tp);
					cp++;
				}
				rint('z', tp);
				break;
				}

			case TR_EVENT:
				/*
				 * Vuid interface and kbd spec differ in that
				 * the vuid doesn't have any events defined
				 * after BOTTOMFUNC+1.  So, we just send the
				 * esc sequence instead of the function key
				 * event.
				 */
				if (entry > BOTTOMFUNC+1) {
					if (newstate == RELEASED)
						break;
					cp = strsetwithdecimal(&buf[0],
					    (u_int)entry, sizeof (buf) - 1);
					kbd_send_esc_event('\033', tp); /* Esc*/
					kbd_send_esc_event('[', tp);
					while (*cp != '\0') {
						kbd_send_esc_event(*cp, tp);
						cp++;
					}
					kbd_send_esc_event('z', tp);
				} else {
					/*
					 * Take advantage of the similar
					 * ordering of kbd.h function keys and
					 * vuid_event.h function keys to do a
					 * simple translation to achieve a
					 * mapping between the 2 different
					 * address spaces.
					 */
					fe.id = entry-LEFTFUNC+KEY_LEFTFIRST;
					fe.value = 1;
					/*
					 * Assume "up" table only generates
					 * shift changes.
					 */
					kbdkeypressed(tp, key, &fe);
					/*
					 * Function key events can be expanded
					 * by terminal emulator software to
					 * produce the standard escape sequence
					 * generated by the TR_ASCII case above
					 * if a function key event is not used
					 * by terminal emulator software
					 * directly.
					 */
				}
				break;
			}
		}
	badentry:
		break;
	}
}

static void
kbd_send_esc_event(c, tp)
	char c;
	struct	tty *tp;
{
	Firm_event fe;

	fe.id = c;
	fe.value = 1;
	fe.pair_type = FE_PAIR_NONE;
	fe.pair = 0;
	/*
	 * Pretend as if each cp pushed and released
	 * Calling kbdqueueevent avoids addr translation
	 * and pair base determination of kbdkeypressed.
	 */
	kbdqueueevent(tp, &fe);
	fe.value = 0;
	kbdqueueevent(tp, &fe);
}

char *
strsetwithdecimal(buf, val, maxdigs)
	char	*buf;
	u_int	val, maxdigs;
{
	int	hradix = 5;
	char	*bp;
	int	lowbit;
	char	*tab = "0123456789abcdef";

	bp = buf + maxdigs;
	*(--bp) = '\0';
	while (val) {
		lowbit = val & 1;
		val = (val >> 1);
		*(--bp) = tab[val % hradix * 2 + lowbit];
		val /= hradix;
	}
	return (bp);
}

kbdkeypressed(tp, key_station, fe)
	struct	tty *tp;
	u_char	key_station;
	Firm_event *fe;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct keyboardstate *k;
	register struct keyboard *kp;
	register u_char base;
	register short id_addr;

	if (kbdd == 0)
		return;
	k = &kbdd->kbdd_state;
	kp = k->k_curkeyboard;
	if (kp == NULL)
		return;
	/* Set pair values */
	if (fe->id < VKEY_FIRST) {
		/* Strip buckybits */
		base = fe->id & 0x7F;
		/* Find base that is the physical keyboard if CTRLed */
		if (k->k_shiftmask & (CTRLMASK | CTLSMASK)) {
			struct keymap *km;

			/* Order of these tests same as in settable */
			if (k->k_shiftmask & SHIFTMASK)
				km = kp->k_shifted;
			else if (k->k_shiftmask & CAPSMASK)
				km = kp->k_caps;
			else
				/* CTRLed keyboard maps into normal for base */
				km = kp->k_normal;
			if (km == NULL)
				/* Gross error */
				km = kp->k_normal;
			base = km->keymap[key_station];
		}
		if (base != fe->id) {
			fe->pair_type = FE_PAIR_SET;
			fe->pair = base;
			goto send;
		}
	}
	fe->pair_type = FE_PAIR_NONE;
	fe->pair = 0;
send:
	/* Adjust event id address for multiple keyboard/workstation support */
	switch (vuid_id_addr(fe->id)) {
	case ASCII_FIRST:
		id_addr = kbdd->kbdd_ascii_addr;
		break;
	case TOP_FIRST:
		id_addr = kbdd->kbdd_top_addr;
		break;
	case VKEY_FIRST:
		id_addr = kbdd->kbdd_vkey_addr;
		break;
	default:
		id_addr = vuid_id_addr(fe->id);
	}
	fe->id = vuid_id_offset(fe->id) | id_addr;
	kbdqueuepress(tp, key_station, fe);
}

static
kbdqueuepress(tp, key_station, fe)
	struct	tty *tp;
	u_char key_station;
	Firm_event *fe;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct key_event *ke;
	register int i;

	if (key_station == IDLEKEY)
		return;
#ifdef	KBD_DEBUG
	if (kbd_input_debug) printf("KBD PRESSED key=%d\n", key_station);
#endif
	/* Scan table of down key stations */
	if (kbdd->kbdd_translate == TR_EVENT ||
	    kbdd->kbdd_translate == TR_UNTRANS_EVENT) {
		for (i = 0, ke = kbdd->kbdd_downs;
		     i < kbdd->kbdd_downs_entries;
		     i++, ke++) {
			/* Keycode already down? */
			if (ke->key_station == key_station) {
#ifdef	KBD_DEBUG
	printf("kbd: Double entry in downs table (%d,%d)!\n", key_station, i);
#endif	KBD_DEBUG
				goto add_event;
			}
			if (ke->key_station == 0)
				goto add_event;
		}
		printf("kbd: Too many keys down!\n");
		ke = kbdd->kbdd_downs;
	}
add_event:
	ke->key_station = key_station;
	ke->event = *fe;
	kbdqueueevent(tp, fe);
}

kbdkeyreleased(tp, key_station)
	struct tty *tp;
	u_char      key_station;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct key_event *ke;
	register int i;

	if (kbdd == 0 || key_station == IDLEKEY)
		return;
#ifdef	KBD_DEBUG
	if (kbd_input_debug)
		printf("KBD RELEASE key=%d\n", key_station);
#endif
	if (kbdd->kbdd_translate != TR_EVENT &&
	    kbdd->kbdd_translate != TR_UNTRANS_EVENT)
		return;
	/* Scan table of down key stations */
	for (i = 0, ke = kbdd->kbdd_downs;
	     i < kbdd->kbdd_downs_entries;
	     i++, ke++) {
		/* Found? */
		if (ke->key_station == key_station) {
			ke->key_station = 0;
			ke->event.value = 0;
			kbdqueueevent(tp, &ke->event);
		}
	}

	/*
	 * Ignore if couldn't find because may be called twice
	 * for the same key station in the case of the kbdrpt 
	 * routine being called unnecessarily. 
	 */
	return;
}

kbdreleaseall(tp)
	struct	tty *tp;
{
	struct kbddata *kbdd = kbdtptokbdd(&tp);
	register struct key_event *ke;
	register int i;

	if (kbdd == 0)
		return;
#ifdef	KBD_DEBUG
	if (kbd_debug && kbd_ra_debug) printf("KBD RELEASE ALL\n");
#endif
	/* Scan table of down key stations */
	for (i = 0, ke = kbdd->kbdd_downs;
	    i < kbdd->kbdd_downs_entries; i++, ke++) {
		/* Key station not zero */
		if (ke->key_station)
			kbdkeyreleased(tp, ke->key_station);
			/* kbdkeyreleased resets kbdd_downs entry */
	}
}

kbdqueueevent(tp, fe)
	struct	tty *tp;
	Firm_event *fe;
{
	struct kbddata *kbdd = kbdtptokbdd(&tp);

	if (kbdd == 0)
		return;
	uniqtime(&fe->time);
	if (vq_put(&kbdd->kbdd_q, fe) == VUID_Q_OVERFLOW) {
		if (kbd_overflow_msg)
			printf("kbd: Buffer flushed when overflowed.\n");
		kbdflush(tp);
		kbd_overflow_cnt++;
	}
	if (vq_used(&kbdd->kbdd_q) == 1)
		/* Place character on tty raw input queue to trigger select */
		ttyinput('\0', tp);
}

kbdread(tp, uio)
	struct	tty *tp;
	struct uio *uio;
{
	register struct kbddata *kbdd = kbdtptokbdd(&tp);
	register int error, pri;
	Firm_event fe;

	if (kbdd == 0)
		return (EINVAL);
	pri = spl5();
	/*
	 * Wait on tty raw queue if this queue is empty since the tty is
	 * controlling the select/wakeup/sleep stuff.
	 */
	while (tp->t_rawq.c_cc <= 0) {
		if (tp->t_state&TS_NBIO) {
			(void) splx(pri);
			return (EWOULDBLOCK);
		}
		(void) sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	switch (kbdd->kbdd_translate) {

	case TR_EVENT:
	case TR_UNTRANS_EVENT:
		while (vq_peek(&kbdd->kbdd_q, &fe) != VUID_Q_EMPTY) {
			if (uio->uio_resid < sizeof (Firm_event))
				goto done;
			/* lower pri to avoid mouse droppings */
			(void) splx(pri);
			error = uiomove((caddr_t)&fe, sizeof(fe), UIO_READ, uio);
			/* spl5 should return same priority as pri */
			pri = spl5();
			if (error)
				break;
			(void) vq_get(&kbdd->kbdd_q, &fe);
		}
		/* Flush tty if no more to read */
		if (vq_is_empty(&kbdd->kbdd_q))
			ttyflush(tp, FREAD);
		break;

	case TR_ASCII:
	case TR_NONE:
		while (tp->t_rawq.c_cc && uio->uio_resid) {
			/* lower pri to avoid mouse droppings? */
			(void) splx(pri);
			error = ureadc(getc(&tp->t_rawq), uio);
			/* spl5 should return same priority as pri */
			pri = spl5();
			if (error)
				break;
		}
		break;
	}
done:
	/* Release protection AFTER ttyflush or will get out of sync with tty */
	(void) splx(pri);
	return (error);
}
