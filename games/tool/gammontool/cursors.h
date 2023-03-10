/*	@(#)cursors.h 1.1 86/09/24 SMI */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

static short	blank_data[16] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};
mpr_static(blank_pr, 16, 16, 1, blank_data);

static short	fist_data[16] = {
	0x0000, 0x0000, 0x0140, 0x07E0, 0x0FE0, 0x0FF0, 0x1FF0, 0x0FF0,
	0x07FE, 0x03E2, 0x0181, 0x0083, 0x0126, 0x008C, 0x0078, 0x0020
};
mpr_static(fist_pr, 16, 16, 1, fist_data);

static short	pointer_data[16] = {
	0x0020, 0x0070, 0x009c, 0x0106, 0x0083, 0x01c9, 0x03e2, 0x07fe,
        0x0ff0, 0x1ff0, 0x3ff0, 0x3fe0, 0x77e0, 0x6140, 0xe000, 0xc000
};
mpr_static(pointer_pr, 16, 16, 1, pointer_data);

static short	hrglass_data[16] = {
	0x3FFC, 0x2004, 0x300C, 0x3C3C, 0x1FF8, 0x0FF0, 0x03C0, 0x0180,
	0x0180, 0x03C0, 0x0DB0, 0x1188, 0x2184, 0x23C4, 0x27E4, 0x3FFC
};
mpr_static(hrglass_pr, 16, 16, 1, hrglass_data);

static short	dollar_data[16] = {
	0x0240, 0x0240, 0x0FF0, 0x1248, 0x1240, 0x1240, 0x1240, 0x0FF0,
	0x0248, 0x0248, 0x0248, 0x1248, 0x0FF0, 0x0240, 0x0240, 0x0000
};
mpr_static(dollar_pr, 16, 16, 1, dollar_data);

static struct cursor cursors[5] = {
	{0, 0, PIX_SRC | PIX_DST, &blank_pr},	/* will be filled in later */
	{7, 7, PIX_SRC | PIX_DST, &fist_pr},
	{0, 16, PIX_SRC | PIX_DST, &pointer_pr},
	{7, 7, PIX_SRC | PIX_DST, &hrglass_pr},
	{7, 7, PIX_SRC | PIX_DST, &dollar_pr}
};
