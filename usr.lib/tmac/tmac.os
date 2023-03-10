.\" @(#)tmac.os 1.1 86/09/25 SMI; from UCB 4.2
.ds // /usr/lib/ms/
.	\" RT - reset (at new paragraph)
.de RT
.if !\\n(1T .BG
.if !\\n(IK .if !\\n(IF .if !\\n(IX .if !\\n(BE .di
.if \\n(TM .ls 2
.ce 0
.ul 0
.if \\n(QP \{\
.	ll +\\n(QIu
.	in -\\n(QIu
.	nr QP -1
.\}
.if \\n(NX<=1 .if \\n(AJ=0 .ll \\n(LLu
.if \\n(IF=0 \{\
.	ps \\n(PS
.	if \\n(VS>=40 .vs \\n(VSu
.	if \\n(VS<=39 .vs \\n(VSp
.\}
.if \\n(IP .in -\\n(I\\n(IRu
.if \\n(IP=0 .nr I0 \\n(PIu
.if \\n(IP .nr IP -1
.ft 1
.bd 1
.TA
.fi
..
.	\" IZ - initialize (before text begins)
.de IZ
.nr FM 1i
.nr YY -\\n(FMu
.nr XX 0 1
.nr IP 0
.nr PI 5n
.nr QI 5n
.nr FI 2n
.nr I0 \\n(PIu
.if n .nr PD 1v
.if t .nr PD 0.3v
.nr PS 10
.nr VS 12
.ps \\n(PS
.vs \\n(VSp
.nr ML 3v
.nr IR 0
.nr TB 0
.nr SJ \\n(.j
.nr PO \\n(.o
.nr LL 6i
.ll \\n(LLu
.lt 6i
.ev 1
.nr FL 5.5i
.ll \\n(FLu
.ps 8
.vs 10p
.ev
.ds CH - \\\\n(PN -
.if n .ds CF \\*(DY
.wh 0 NP
.wh -\\n(FMu FO
.ch FO 16i
.wh -\\n(FMu FX
.ch FO -\\n(FMu
.wh -\\n(FMu/2u BT
..
.	\" TA - set default tabs
.de TA
.if n .ta 8n 16n 24n 32n 40n 48n 56n 64n 72n 80n
.if t .ta 5n 10n 15n 20n 25n 30n 35n 40n 45n 50n 55n 60n 65n 70n 75n
..
.	\" RP - released paper format
.de RP
.nr ST 2
.if "\\$1"no" .nr ST 1
.pn 0
.br
..
.	\" TL - source file for cover sheet
.de TL
.rn TL @T
.so \*(//ms.cov
.TL
.rm @T
..
.	\" TS - source file for tbl
.de TS
.rn TS @T
.so \*(//ms.tbl
.TS \\$1 \\$2
.rm @T
..
.	\" EQ - source file for eqn
.de EQ
.rn EQ @T
.so \*(//ms.eqn
.EQ \\$1 \\$2
.rm @T
..
.	\" ]- - source file for refer
.de ]-
.rn ]- @T
.so \*(//ms.ref
.]-
.rm @T
..
.	\" [< - for refer -s or -e
.de ]<
.rn ]< @T
.so \*(//ms.ref
.]<
.rm @T
..
.if n .ds Q \&"
.if n .ds U \&"
.if t .ds Q ``
.if t .ds U ''
.if n .ds - --
.if t .ds - \(em
.if \n(.V>19 .ds [. \f1[
.if \n(.V<20 .ds [. \f1\s-2\v'-0.4m'
.if \n(.V>19 .ds .] ]\fP
.if \n(.V<20 .ds .] \v'0.4m'\s+2\fP
.ds <. .
.ds <, ,
.	\" PP - regular paragraph
.de PP
.RT
.ne 1.1
.if \\n(1T .sp \\n(PDu
.ti +\\n(PIu
..
.	\" LP - left aligned paragraph
.de LP
.RT
.ne 1.1
.if \\n(1T .sp \\n(PDu
.ti \\n(.iu
..
.	\" IP - indented paragraph
.de IP
.RT
.br
.ne 2.1v
.sp \\n(PDu
.if !\\n(IP .nr IP +1
.if \\n(.$-1 .nr I\\n(IR \\$2n
.in +\\n(I\\n(IRu
.ta \\n(I\\n(IRu
.if \\n(.$>0 \{\
.ti -\\n(I\\n(IRu
\&\\$1\t\c
.\}
..
.	\" XP - exdented paragraph
.de XP
.RT
.ne 1.1
.if \\n(1T .sp \\n(PDu
.if !\\n(IP .nr IP +1
.in +\\n(I\\n(IRu
.ti -\\n(I\\n(IRu
..
.	\" QP - quote paragraph
.de QP
.ti \\n(.iu
.RT
.if \\n(1T .sp \\n(PDu
.ne 1.1
.nr QP 1
.in +\\n(QIu
.ll -\\n(QIu
.ti \\n(.iu
.if \\n(TM .ls 1
..
.	\" SH - section header
.de SH
.ti \\n(.iu
.RT
.if \\n(1T .sp
.if !\\n(1T .BG
.RT
.ne 3.1
.ft 3
.if n .ul 1000
..
.	\" NH - source file for numbered header
.de NH
.rn NH @T
.so \*(//ms.toc
.NH \\$1 \\$2 \\$3 \\$4 \\$5 \\$6
.rm @T
..
.	\" BG - begin (at first paragraph)
.de BG
.br
.nr YE 1
.di
.ce 0
.nr KI 0
.hy 14
.nr 1T 1
.S\\n(ST
.rm S2 TX AX WT MF RP
.rm I1 I2 I3 I4 I5
.de TL
.ft 3
.sp
.if n .ul 100
.ce 100
.LG
\\..
.de AU
.ft 2
.if n .ul 0
.ce 100
.sp
.NL
\\..
.de AI
.ft 1
.ce 100
.if n .ul 0
.if n .sp
.if t .sp .5
.NL
\\..
.RA
.rm RA
.rn FJ FS
.rn FK FE
.nf
.ev 1
.ps \\n(PS-2
.vs \\n(.s+2p
.ev
.if \\n(KG=0 .nr FP 0 
.nr KG 0 
.if \\n(FP>0 \{\
.	FS
.	FG
.	FE
.\}
.br
.if \\n(TV>0 .if n .sp 2
.if \\n(TV>0 .if t .sp 1
.fi
.ll \\n(LLu
..
.	\" RA - redefine abstract macros
.de RA
.de AB
.br
.if !\\n(1T .BG
.ce
.sp
.if \\n(.$=0 ABSTRACT
.if \\n(.$>0 .if !"\\$1"-" .if !"\\$1"no" \\$1
.if \\n(.$=0 .sp
.if \\n(.$>0 .if !"\\$1"-" .if !"\\$1"no" .sp
.sp
.nr AJ 1
.in +\\n(.lu/12u
.ll -\\n(.lu/12u
.RT
.if \\n(TM .ls 1
\\..
.de AE
.nr AJ 0
.br
.in 0
.ll \\n(LLu
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
.if \\n(TM .ls 2
\\..
..
.	\" DS - display with keep (L left, I indent, C center, B block)
.de DS
.KS
.nf
.\\$1D \\$2 \\$1
.ft 1
.ps \\n(PS
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
..
.de D
.ID \\$1
..
.	\" ID - indented display with no keep
.de ID
.XD
.if t .in +0.5i
.if n .in +8
.if \\n(.$ .if !"\\$1"I" .if !"\\$1"" \{\
.	in \\n(OIu
.	in +\\$1n
.\}
..
.	\" LD - left display with no keep
.de LD
.XD
..
.	\" CD - centered display with no keep
.de CD
.XD
.ce 1000
..
.	\" XD - real display macro
.de XD
.nf
.nr OI \\n(.i
.if t .sp 0.5
.if n .sp 1
.if \\n(TM .ls 1
..
.	\" DE - end display of any kind
.de DE
.ce 0
.if \\n(BD>0 .DF
.nr BD 0
.in \\n(OIu
.KE
.if \\n(TM .ls 2
.if t .sp 0.5
.if n .sp 1
.fi
..
.	\" BD - block display: center entire block
.de BD
.XD
.nr BD 1
.nf
.in \\n(OIu
.di DD
..
.	\" DF - finish block display
.de DF
.di
.if \\n(dl>\\n(BD .nr BD \\n(dl
.if \\n(BD<\\n(.l .in (\\n(.lu-\\n(BDu)/2u
.nr EI \\n(.l-\\n(.i
.ta \\n(EIuR
.DD
.in \\n(OIu
..
.	\" KS - begin regular keep
.de KS
.nr KN \\n(.u
.if \\n(IK=0 .if \\n(IF=0 .KQ
.nr IK +1
..
.	\" KQ - real keep processor
.de KQ
.br
.nr KI \\n(.i
.ev 2
.TA
.br
.in \\n(KIu
.ps \\n(PS
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
.ll \\n(LLu
.lt \\n(LTu
.if \\n(NX>1 .ll \\n(CWu
.if \\n(NX>1 .lt \\n(CWu
.di KK
.nr TB 0
..
.	\" KF - begin floating keep
.de KF
.nr KN \\n(.u
.if !\\n(IK .FQ
.nr IK +1
..
.	\" FQ - real floating keep processor
.de FQ
.nr KI \\n(.i
.ev 2
.TA
.br
.in \\n(KIu
.ps \\n(PS
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
.ll \\n(LLu
.lt \\n(LTu
.if \\n(NX>1 .ll \\n(CWu
.if \\n(NX>1 .lt \\n(CWu
.di KK
.nr TB 1
..
.	\" KE - end keep
.de KE
.if \\n(IK .if !\\n(IK-1 .if \\n(IF=0 .RQ
.if \\n(IK .nr IK -1
..
.	\" RQ - real keep release
.de RQ
.br
.di
.nr NF 0
.if \\n(dn-\\n(.t .nr NF 1
.if \\n(TC .nr NF 1
.if \\n(NF .if !\\n(TB .sp 200
.if !\\n(NF .if \\n(TB .nr TB 0
.nf
.rs
.nr TC 5
.in 0
.ls 1
.if \\n(TB=0 \{\
.	ev
.	br
.	ev 2
.	KK
.\}
.ls
.ce 0
.if \\n(TB=0 .rm KK
.if \\n(TB .da KJ
.if \\n(TB \!.KD \\n(dn
.if \\n(TB .KK
.if \\n(TB .di
.nr TC \\n(TB
.if \\n(KN .fi
.in
.ev
..
.	\" KD - keep redivert
.de KD
.nr KM 0
.if "\\n(.z"KJ" .nr KM 1
.if \\n(KM>0 \!.KD \\$1
.if \\n(KM=0 .if \\n(.t<\\$1 .di KJ
..
.	\" EM - end macro (process leftover keep)
.de EM
.br
.if \\n(TB=0 .if t .wh -1p CM
.if \\n(TB \&\c
.if \\n(TB 'bp
.if \\n(TB .NP
.if \\n(TB .ch CM 160
..
.de XK
.nr TD 1
.nf
.ls 1
.in 0
.rn KJ KL
.KL
.rm KL
.if "\\n(.z"KJ" .di
.nr TB 0
.if "\\n(.z"KJ" .nr TB 1
.br
.in
.ls
.fi
.nr TD 0
..
.	\" NP - new page
.de NP
.if !\\n(LT .nr LT \\n(LLu
.if \\n(FM+\\n(HM>=\\n(.p \{\
.	tm Margins bigger than page length
.	ab
.\}
.if t .CM
.if \\n(HM=0 .nr HM 1i
.po \\n(POu
.nr PF \\n(.f
.nr PX \\n(.s
.ft 1
.ps \\n(PS
'sp \\n(HMu/2u
.PT
'sp |\\n(HMu
.ps \\n(PX
.ft \\n(PF
.nr XX 0 1
.nr YY 0-\\n(FMu
.ch FO 16i
.ch FX 17i
.ch FO -\\n(FMu
.ch FX \\n(.pu-\\n(FMu
.if \\n(MF .FV
.nr MF 0
.mk
.os
.ev 1
.if \\n(TD=0 .if \\n(TC<5  .XK
.nr TC 0
.ev
.nr TQ \\n(.i
.nr TK \\n(.u
.if \\n(IT>0 \{\
.	in 0
.	nf
.	TT
.	in \\n(TQu
.	if \\n(TK .fi
.\}
.ns
.mk #T
.if t .if \\n(.o+\\n(LL>7.54i .tm PO + LL wider than 7.54i
..
.	\" PT - page titles
.de PT
.lt \\n(LTu
.pc %
.nr PN \\n%
.nr PT \\n%
.if \\n(P1 .nr PT 2
.if \\n(PT>1 .if !\\n(EH .if !\\n(OH .tl '\\*(LH'\\*(CH'\\*(RH'
.if \\n(PT>1 .if \\n(OH .if o .tl \\*(O1
.if \\n(PT>1 .if \\n(EH .if e .tl \\*(E2
.lt \\n(.lu
..
.	\" OH - odd page header
.de OH
.nr OH 1
.if !\\n(.$ .nr OH 0
.ds O1 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" EH - even page header
.de EH
.nr EH 1
.if !\\n(.$ .nr EH 0
.ds E2 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" P1 - do PT on 1st page
.de P1
.nr P1 1
..
.	\" FO - footer
.de FO
.rn FO FZ
.if \\n(IT>0 .nr T. 1
.if \\n(IT>0 .if \\n(FC=0  .T# 1
.if \\n(IT>0 .br
.nr FC +1
.if \\n(NX<2 .nr WF 0
.nr dn 0
.if \\n(FC<=1 .if \\n(XX .XF
.rn FZ FO
.nr MF 0
.if \\n(dn  .nr MF 1
.if !\\n(WF .nr YY 0-\\n(FMu
.if !\\n(WF .ch FO \\n(YYu
.if !\\n(dn .nr WF 0
.if \\n(FC<=1 .if \\n(XX=0 \{\
.	if \\n(NX>1 .RC
.	if \\n(NX<2 'bp
.\}
.nr FC -1
.if \\n(ML>0 .ne \\n(MLu
..
.	\" BT - bottom title
.de BT
.nr PF \\n(.f
.nr PX \\n(.s
.ft 1
.ps \\n(PS
.lt \\n(LTu
.po \\n(POu
.if \\n(TM .if \\n(CT \{\
.	tl ''\\n(PN''
.	nr CT 0
.\}
.if \\n% .if !\\n(EF .if !\\n(OF .tl '\\*(LF'\\*(CF'\\*(RF'
.if \\n% .if \\n(OF .if o .tl \\*(O3
.if \\n% .if \\n(EF .if e .tl \\*(E4
.ft \\n(PF
.ps \\n(PX
..
.	\" OF - odd page footer
.de OF
.nr OF 1
.if !\\n(.$ .nr OF 0
.ds O3 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" EF - even page footer
.de EF
.nr EF 1
.if !\\n(.$ .nr EF 0
.ds E4 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" 2C - double column
.de 2C
.MC
..
.	\" 1C - single column
.de 1C
.MC \\n(LLu
.hy 14
..
.	\" MC - multiple columns, arg is col width
.de MC
.nr L1 \\n(LL*7/15
.if \\n(.$>0 .nr L1 \\$1n
.nr NQ \\n(LL/\\n(L1
.if \\n(NQ<1 .nr NQ 1
.if \\n(NQ>2 .if (\\n(LL%\\n(L1)=0 .nr NQ -1
.if !\\n(1T \{\
.	BG
.	if n .sp 4
.	if t .sp 2
.\}
.if \\n(NX=0 .nr NX 1
.if !\\n(NX=\\n(NQ \{\
.	RT
.	if \\n(NX>1 .bp
.	mk
.	nr NC 1
.	po \\n(POu
.\}
.if \\n(NQ>1 .hy 12
.nr NX \\n(NQ
.nr CW \\n(L1
.ll \\n(CWu
.nr FL \\n(CWu*11u/12u
.if \\n(NX>1 .nr GW (\\n(LL-(\\n(NX*\\n(CW))/(\\n(NX-1)
.nr RO \\n(CW+\\n(GW
.ns
..
.de RC
.if \\n(NC>=\\n(NX .C2
.if \\n(NC<\\n(NX .C1
.nr NC \\n(ND
..
.de C1
.rt
.po +\\n(ROu
.nr ND \\n(NC+1
.nr XX 0 1
.if \\n(MF .FV
.ch FX \\n(.pu-\\n(FMu
.ev 1
.if \\n(TB .XK
.nr TC 0
.ev
.nr TQ \\n(.i
.if \\n(IT>0 .in 0
.if \\n(IT>0 .TT
.if \\n(IT>0 .in \\n(TQu
.mk #T
.ns
..
.de C2
.po \\n(POu
'bp
.nr ND 1
..
.	\" RS - right shift indent
.de RS
.nr IS \\n(IP
.RT
.nr IP \\n(IS
.if \\n(IP>0 .in +\\n(I\\n(IRu
.nr IR +1
.nr I\\n(IR \\n(PIu
.in +\\n(I\\n(IRu
..
.	\" RE - retreat to the left
.de RE
.nr IS \\n(IP
.RT
.nr IP \\n(IS
.if \\n(IR>0 .nr IR -1
.if \\n(IP<=0 .in -\\n(I\\n(IRu
..
.	\" CM - cut mark
.de CM
.po 0
.lt 7.6i
.ft 1
.ps 10
.vs 4p
.tl '--''--'
.po
.vs
.lt
.ps
.ft
..
.	\" I - italic font
.de I
.nr PQ \\n(.f
.if t .ft 2
.if "\\$1"" .if n .ul 1000
.if !"\\$1"" .if n .ul 1
.if t .if !"\\$1"" \&\\$1\|\f\\n(PQ\\$2
.if n .if \\n(.$=1 \&\\$1
.if n .if \\n(.$>1 \&\\$1\c
.if n .if \\n(.$>1 \&\\$2
..
.	\" B - bold font
.de B
.nr PQ \\n(.f
.if t .ft 3
.if "\\$1"" .if n .ul 1000
.if !"\\$1"" .if n .ul 1
.if t .if !"\\$1"" \&\\$1\f\\n(PQ\\$2
.if n .if \\n(.$=1 \&\\$1
.if n .if \\n(.$>1 \&\\$1\c
.if n .if \\n(.$>1 \&\\$2
..
.	\" R - Roman font
.de R
.if n .ul 0
.ft 1
..
.	\" UL - underline in troff
.de UL
.if t \\$1\l'|0\(ul'\\$2
.if n .I \\$1 \\$2
..
.	\" SM - smaller size
.de SM
.ps -2
..
.	\" LG - larger size
.de LG
.ps +2
..
.	\" NL - normal size
.de NL
.ps \\n(PS
..
.if \n(mo-0 .ds MO January
.if \n(mo-1 .ds MO February
.if \n(mo-2 .ds MO March
.if \n(mo-3 .ds MO April
.if \n(mo-4 .ds MO May
.if \n(mo-5 .ds MO June
.if \n(mo-6 .ds MO July
.if \n(mo-7 .ds MO August
.if \n(mo-8 .ds MO September
.if \n(mo-9 .ds MO October
.if \n(mo-10 .ds MO November
.if \n(mo-11 .ds MO December
.ds DY \*(MO \n(dy, 19\n(yr
.IZ
.rm IZ
.em EM
.	\" DA - force date
.de DA
.if \\n(.$ .ds DY \\$1 \\$2 \\$3 \\$4
.ds CF \\*(DY
..
.	\" ND - no date or new date
.de ND
.if \\n(.$ .ds DY \\$1 \\$2 \\$3 \\$4
.rm CF
..
.	\" \** - automatically numbered footnote
.nr * 0 1
.ds * \\*([.\\n+*\\*(.]
.	\" FJ - replaces FS after cover sheet
.de FJ
'ce 0
.di
.ev 1
.ll \\n(FLu
.da FF
.br
.if \\n(IF>0 .tm Footnote within footnote is illegal
.nr IF 1
.if !\\n+(XX-1 .FA
.if \\n(MF=0 .if \\n(.$=0 .if \\n*>0 .FP \\n*
.if \\n(MF=0 .if \\n(.$>0 .FP \\$1 no
..
.	\" FK - replaces FE after cover sheet
.de FK
.br
.in 0
.nr IF 0
.di
.ev
.if !\\n(XX-1 .nr dn +\\n(.v
.nr YY -\\n(dn
.if \\n(NX=0 .nr WF 1
.if \\n(dl>\\n(CW .nr WF 1
.if (\\n(nl+\\n(.v)<=(\\n(.p+\\n(YY) .ch FO \\n(YYu
.if (\\n(nl+\\n(.v)>(\\n(.p+\\n(YY) \{\
.	if \\n(nl>(\\n(HM+1.5v) .ch FO \\n(nlu+\\n(.vu
.	if \\n(nl+\\n(FM+1v>\\n(.p .ch FX \\n(.pu-\\n(FMu+2v
.	if \\n(nl<=(\\n(HM+1.5v) .ch FO \\n(HMu+(4u*\\n(.vu)
.\}
..
.	\" FS - begin footnote (on cover)
.de FS
.ev 1
.br
.ll \\n(FLu
.da FG
.if \\n(.$=0 .if \\n*>0 .FP \\n*
.if \\n(.$>0 .FP \\$1 no
..
.	\" FE - end footnote (on cover)
.de FE
.br
.di
.nr FP \\n(dn
.if \\n(1T=0 .nr KG 1
.ev
..
.	\" FA - print line before footnotes
.de FA
.in 0
.if n _________________________
.if t \l'1i'
.br
..
.	\" FP - footnote paragraph
.de FP
.sp \\n(PDu/2u
.if \\n(FF<2 .ti \\n(FIu
.if \\n(FF=3 \{\
.	in \\n(FIu*2u
.	ta \\n(FIu*2u
.	ti 0
.\}
.if \\n(FF=0 \{\
.	ie "\\$2"no" \\$1\0\c
.	el \\*([.\\$1\\*(.]\0\c
.\}
.if \\n(FF>0 .if \\n(FF<3 \{\
.	ie "\\$2"no" \\$1\0\c
.	el \\$1.\0\c
.\}
.if \\n(FF=3 \{\
.	ie "\\$2"no" \\$1\t\c
.	el \\$1.\t\c
.\}
..
.	\" FV - call back leftover footnote from previous page
.de FV
.FS
.nf
.ls 1
.FY
.ls
.fi
.FE
..
.	\" FX - divert leftover footnote for next page
.de FX
.if \\n(XX>0 .di FY
.if \\n(XX>0 .ns
..
.	\" XF - actually print footnote
.de XF
.if \\n(nlu+1v>(\\n(.pu-\\n(FMu) .ch FX \\n(nlu+1.9v
.ev 1
.nf
.ls 1
.FF
.rm FF
.nr XX 0 1
.br
.ls
.di
.fi
.ev
..
.	\" accent marks
.ds ' \h'\w'e'u*4/10'\z\'\h'-\w'e'u*4/10'
.ds ` \h'\w'e'u*4/10'\z\`\h'-\w'e'u*4/10'
.ds : \v'-0.6m'\h'(1u-(\\n(.fu%2u))*0.13m+0.06m'\z.\h'0.2m'\z.\h'-((1u-(\\n(.fu%2u))*0.13m+0.26m)'\v'0.6m'
.ds ^ \k:\h'-\\n(.fu+1u/2u*2u+\\n(.fu-1u*0.13m+0.06m'\z^\h'|\\n:u'
.ds ~ \k:\h'-\\n(.fu+1u/2u*2u+\\n(.fu-1u*0.13m+0.06m'\z~\h'|\\n:u'
.ds C \k:\h'+\w'e'u/4u'\v'-0.6m'\s6v\s0\v'0.6m'\h'|\\n:u'
.ds , \k:\h'\w'c'u*0.4u'\z,\h'|\\n:u'
.	\" AM - better accent marks
.de AM
.so \*(//ms.acc
..
.	\" TM - thesis mode
.de TM
.so \*(//ms.ths
..
.	\" BX - word in a box
.de BX
.if t \(br\|\\$1\|\(br\l'|0\(rn'\l'|0\(ul'
.if n \(br\\kA\|\\$1\|\\kB\(br\v'-1v'\h'|\\nBu'\l'|\\nAu'\v'1v'\l'|\\nAu'
..
.	\" B1 - source file for boxed text
.de B1
.rn B1 @T
.so \*(//ms.tbl
.B1 \\$1
.rm @T
..
.	\" XS - table of contents
.de XS
.rn XS @T
.so \*(//ms.toc
.XS \\$1 \\$2
.rm @T
..
.	\" IX - index words to stderr
.de IX
.tm \\$1\t\\$2\t\\$3\t\\$4\t\\$5 ... \\n(PN
..
.	\" UX - UNIX macro
.de UX
.ie \\n(UX \s-1UNIX\s0\\$1
.el \{\
\s-1UNIX\s0\\$1\(dg
.FS
\(dg \s-1UNIX\s0 is a trademark of Bell Laboratories.
.FE
.nr UX 1
.\}
..
.rm //
