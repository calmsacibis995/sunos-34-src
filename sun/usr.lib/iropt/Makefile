#	@(#)Makefile 1.1 86/09/25 SMI	

DBX=
CFLAGS= -O $(DBX) -DIROPT
IROPT=iropt
DESTDIR=
HDRS= iropt.h ir_common.h opdescr.h loop.h reg.h page.h
ASM_SRCS = \
	bit_util.s
C_SRCS = \
	iropt.c misc.c read_ir.c var_df.c cf.c expr_df.c \
	recordrefs.c page.c loop.c li.c ln.c iv.c reg.c reg_alloc.c cse.c \
	copy_ppg.c copy_df.c implicit.c make_expr.c debug.c ir_wf.c
SCCS_FILES = Makefile $(ASM_SRCS) $(C_SRCS) $(HDRS)
OBJS= \
	iropt.o misc.o read_ir.o var_df.o cf.o expr_df.o \
	recordrefs.o page.o loop.o li.o ln.o iv.o reg.o reg_alloc.o cse.o \
	copy_ppg.o copy_df.o implicit.o make_expr.o \
	bit_util.o debug.o ir_wf.o

iropt:	$(OBJS)
	$(CC) $(DBX) -o $(IROPT) $(OBJS)
	
bit_util.o:	bit_util.s
# define -DPROF if profiling
	/lib/cpp bit_util.s > tmp$$$$.s; as -o bit_util.o tmp$$$$.s; rm tmp$$$$.s

install : iropt
	install -s $(IROPT) $(DESTDIR)/usr/lib/$(IROPT)

sccs_files:; @echo $(SCCS_FILES)

clean:; rm -f $(OBJS) iropt

iropt.h : ir_common.h
	touch -f iropt.h
$(OBJS) : iropt.h opdescr.h
loop.o li.o	ln.o iv.o : loop.h
reg.o reg_alloc.o : reg.h page.h
qu.o recordrefs.o make_expr.o : page.h
