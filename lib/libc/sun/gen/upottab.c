#ifndef lint
static	char sccsid[] = "@(#)upottab.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "unpkd.h"

struct unpkd _d_pot[] = {
	{     0, 0X80000000, 0X00000000 },
	{     3, 0Xa0000000, 0X00000000 },
	{     6, 0Xc8000000, 0X00000000 },
	{     9, 0Xfa000000, 0X00000000 },
	{    13, 0X9c400000, 0X00000000 },
	{    16, 0Xc3500000, 0X00000000 },
	{    19, 0Xf4240000, 0X00000000 },
	{    23, 0X98968000, 0X00000000 },
	{    26, 0Xbebc2000, 0X00000000 },
	{    29, 0Xee6b2800, 0X00000000 },
	{    33, 0X9502f900, 0X00000000 },
	{    36, 0Xba43b740, 0X00000000 },
	{    39, 0Xe8d4a510, 0X00000000 },
	{    43, 0X9184e72a, 0X00000000 },
	{    46, 0Xb5e620f4, 0X80000000 },
	{    49, 0Xe35fa931, 0Xa0000000 },
	{    53, 0X8e1bc9bf, 0X04000000 },
	{    56, 0Xb1a2bc2e, 0Xc5000000 },
	{    59, 0Xde0b6b3a, 0X76400000 },
	{    63, 0X8ac72304, 0X89e80000 },
	{    66, 0Xad78ebc5, 0Xac620000 },
	{    69, 0Xd8d726b7, 0X177a8000 },
	{    73, 0X87867832, 0X6eac9000 },
	{    76, 0Xa968163f, 0X0a57b400 },
	{    79, 0Xd3c21bce, 0Xcceda100 },
	{    83, 0X84595161, 0X401484a0 },
	{    86, 0Xa56fa5b9, 0X9019a5c8 },
	{    89, 0Xcecb8f27, 0Xf4200f3a },
};
struct unpkd _d_big_pot[] = {
	{     0, 0X80000000, 0X00000000 },
	{    93, 0X813f3978, 0Xf8940984 },
	{   186, 0X82818f12, 0X81ed44a0 },
	{   279, 0X83c7088e, 0X1aab65db },
	{   372, 0X850fadc0, 0X9923329e },
	{   465, 0X865b8692, 0X5b9bc5c2 },
	{   558, 0X87aa9aff, 0X79042287 },
	{   651, 0X88fcf317, 0Xf22241e2 },
	{   744, 0X8a5296ff, 0Xe33cc930 },
	{   837, 0X8bab8eef, 0Xb6409c1a },
	{   930, 0X8d07e334, 0X55637eb3 },
	{  1023, 0X8e679c2f, 0X5e44ff8f },
	{  1116, 0X8fcac257, 0X558ee4e6 },
};
struct unpkd _d_r_pot[] = {
	{     0, 0X80000000, 0X00000000 },
	{    -4, 0Xcccccccc, 0Xcccccccd },
	{    -7, 0Xa3d70a3d, 0X70a3d70a },
	{   -10, 0X83126e97, 0X8d4fdf3b },
	{   -14, 0Xd1b71758, 0Xe219652c },
	{   -17, 0Xa7c5ac47, 0X1b478423 },
	{   -20, 0X8637bd05, 0Xaf6c69b6 },
	{   -24, 0Xd6bf94d5, 0Xe57a42bc },
	{   -27, 0Xabcc7711, 0X8461cefd },
	{   -30, 0X89705f41, 0X36b4a597 },
	{   -34, 0Xdbe6fece, 0Xbdedd5bf },
	{   -37, 0Xafebff0b, 0Xcb24aaff },
	{   -40, 0X8cbccc09, 0X6f5088cc },
	{   -44, 0Xe12e1342, 0X4bb40e13 },
	{   -47, 0Xb424dc35, 0X095cd80f },
	{   -50, 0X901d7cf7, 0X3ab0acd9 },
	{   -54, 0Xe69594be, 0Xc44de15b },
	{   -57, 0Xb877aa32, 0X36a4b449 },
	{   -60, 0X9392ee8e, 0X921d5d07 },
	{   -64, 0Xec1e4a7d, 0Xb69561a5 },
	{   -67, 0Xbce50864, 0X92111aeb },
	{   -70, 0X971da050, 0X74da7bef },
	{   -74, 0Xf1c90080, 0Xbaf72cb1 },
	{   -77, 0Xc16d9a00, 0X95928a27 },
	{   -80, 0X9abe14cd, 0X44753b53 },
	{   -84, 0Xf79687ae, 0Xd3eec551 },
	{   -87, 0Xc6120625, 0X76589ddb },
	{   -90, 0X9e74d1b7, 0X91e07e48 },
};
struct unpkd _d_r_big_pot[] = {
	{     0, 0X80000000, 0X00000000 },
	{   -94, 0Xfd87b5f2, 0X8300ca0e },
	{  -187, 0Xfb158592, 0Xbe068d2f },
	{  -280, 0Xf8a95fcf, 0X88747d94 },
	{  -373, 0Xf64335bc, 0Xf065d37d },
	{  -466, 0Xf3e2f893, 0Xdec3f126 },
	{  -559, 0Xf18899b1, 0Xbc3f8ca2 },
	{  -652, 0Xef340a98, 0X172aace5 },
	{  -745, 0Xece53cec, 0X4a314ebe },
	{  -838, 0Xea9c2277, 0X23ee8bcb },
	{  -931, 0Xe858ad24, 0X8f5c22ca },
	{ -1024, 0Xe61acf03, 0X3d1a45df },
	{ -1117, 0Xe3e27a44, 0X4d8d98b8 },
};
#if 0
/* THIS IS THE BC PROGRAM USED TO GENERATE THE VALUES IN _d_r_big_pot[] */
/*
 * 
 * for some power p and some number n, calculate
 * the series 10^0 10^-p ... 10^-(p*(n-1))
 *
 * since dc cannot hack these really small values, we scale the
 * running value, remembering that 10^x = (5/4)^x * 2^(3*x)
 * Things are also complicated by our wanting to output the things
 * as normalized hexidecimals, decimal point just to the right of
 * the first binary digit. That is, we want 1.0 output as:
 * 0x80000000000.
 */
/*
 * parameters are:
 * 	p: delta-power-of-ten
 * 	n: number of them to compute
 * variables are:
 *	f: (5/4)^-p
 *	d: 3*p
 *	m: mantissa
 *	e: exponent
 *	i: current iteration
 *
 * we have to be a little careful of the scale, since dc seems to have
 * bugs relating to scale and value of numbers.
 */

p = 28
n = 13

scale = 4
f = 5/4
scale = 50
f = f ^ -p
d = 3*p
/* initially, 10^0 = 1 * 2^ 0 */
m = 0.5
e = 0

for( i = 0 ; i < n ; i ++ ){
	/* normalize and print i-th term */
	while( m < 0.5 ){
	    m *= 2
	    e += 1
	}
	obase = 10
	-e
	obase = 16
	m

	/* compute next value in series */
	m *= f
	e += d
}
#endif 0
