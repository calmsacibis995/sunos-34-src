"	@(#)gmon.ex 1.1 86/09/24 SMI; from UCB 4.1 82/12/10
"	fix funny things done by mcount()"
"	fix its name, make it jsb'able, change registers to protect the caller"
g/_mcount/s//mcount/g
/mcount:/-
/.word/s/.word.*//
/the beginning of mcount()/mark a
/the end of mcount()/mark b
'a,'bs/r11/r5/g
'a,'bs/r10/r4/g
'a,'bs/r9/r3/g
'a,'bs/r8/r2/g
'a,'bs/r7/r1/g
""
"	fix funny name for curbrk used by brk()"
g/_curbrk/s//curbrk/g
w
q
