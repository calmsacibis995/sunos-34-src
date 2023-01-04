|	.asciz	"@(#)fswitch.s 1.1 86/09/25 Copyr 1986 Sun Micro"
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

|	-fswitch inline unexpansion file.

|	Single Precision Complex libF77 routines

        .inline Fc_add,0
	jsr	Fc_add
        .end

        .inline Fc_minus,0
	jsr	Fc_minus
        .end

	.inline	Fc_mult,0
	jsr	Fc_mult
	.end

	.inline Fc_ne,0
	jsr	Fc_ne
	.end
	
	.inline Fc_eq,0
	jsr	Fc_eq
	.end

|	Double Precision Complex libF77 routines

	.inline	Fz_add,0
	jsr	__z_add
	.end

	.inline	__z_add,0
	jsr	__z_add
	.end

	.inline	Fz_minus,0
	jsr	__z_minus
	.end

	.inline	__z_minus,0
	jsr	__z_minus
	.end

        .inline Fz_conv_c,0
	jsr	Fz_conv_c
	.end

	.inline	Fc_conv_z,0
	jsr	Fc_conv_z
	.end

