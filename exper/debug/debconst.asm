; Copyright (C) 1983 Microsoft Corp.
; Modifications copyright 2018 John Elliott
;           and copyright 2022 S. V. Nickolas.
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the Software), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
; IN THE SOFTWARE.
;
; MS-DOS is a Registered Trademark of Microsoft Corp.

include debequ.inc
include dossym.inc

code      segment   public byte 'code'
code      ends

const     segment   public byte
const     ends

data      segment   public byte
data      ends

dg        group     code,const,data

code      segment   public  byte 'code'

          extrn         alufromreg:near,alutoreg:near,accimm:near
          extrn         segop:near,espre:near,sspre:near,cspre:near
          extrn         dspre:near,regop:near,nooperands:near
          extrn         savhex:near,shortjmp:near,movsegto:near
          extrn         wordtoalu:near,movsegfrom:near,getaddr:near
          extrn         xchgax:near,longjmp:near,loadacc:near,storeacc:near
          extrn         regimmb:near,sav16:near,memimm:near,int3:near,sav8:near
          extrn         chk10:near,m8087:near,m8087_d9:near,m8087_db:near
          extrn         m8087_dd:near,m8087_df:near,infixb:near,infixw:near
          extrn         outfixb:near,outfixw:near,jmpcall:near,invarb:near
          extrn         invarw:near,outvarb:near,outvarw:near,prefix:near
          extrn         immed:near,signimm:near,shift:near,shiftv:near
          extrn         grp1:near,grp2:near,regimmw:near


          extrn         db_oper:near,dw_oper:near,assemloop:near,group2:near
          extrn         no_oper:near,group1:near,fgroupp:near,fgroupx:near
          extrn         fgroupz:near,fd9_oper:near,fgroupb:near,fgroup:near
          extrn         fgroupds:near,dcinc_oper:near,int_oper:near,in_oper:near
          extrn         disp8_oper:near,jmp_oper:near,l_oper:near,mov_oper:near
          extrn         out_oper:near,push_oper:near,get_data16:near
          extrn         fgroup3:near,fgroup3w:near,fde_oper:near,esc_oper:near
          extrn         aa_oper:near,call_oper:near,fdb_oper:near,pop_oper:near
          extrn         rotop:near,tst_oper:near,ex_oper:near

code      ends

const     segment   public byte

          public    reg8,reg16,sreg,siz8,distab,dbmn,addmn,adcmn,submn
          public    sbbmn,xormn,ormn,andmn,aaamn,aadmn,aasmn,callmn,cbwmn
          public    upmn,dimn,cmcmn,cmpmn,cwdmn,daamn,dasmn,decmn,divmn
          public    escmn,hltmn,idivmn,imulmn,incmn,intomn,intmn,inmn,iretmn
          public    jamn,jcxzmn,jncmn,jbemn,jzmn,jgemn,jgmn,jlemn,jlmn,jmpmn
          public    jnzmn,jpemn,jnzmn,jpemn,jpomn,jnsmn,jnomn,jomn,jsmn,lahfmn
          public    ldsmn,leamn,lesmn,lockmn,lodbmn,lodwmn,loopnzmn,loopzmn
          public    loopmn,movbmn,movwmn,movmn,mulmn,negmn,nopmn,notmn,outmn
          public    popfmn,popmn,pushfmn,pushmn,rclmn,rcrmn,repzmn,repnzmn
          public    retfmn,retmn,rolmn,rormn,sahfmn,sarmn,scabmn,scawmn,shlmn
          public    shrmn,stcmn,downmn,eimn,stobmn,stowmn,testmn,waitmn,xchgmn
          public    xlatmn,essegmn,cssegmn,sssegmn,dssegmn,badmn

          public    m8087_tab,fi_tab,size_tab,md9_tab,md9_tab2,mdb_tab
          public    mdb_tab2,mdd_tab,mdd_tab2,mdf_tab,optab,maxop,shftab,immtab
          public    grp1tab,grp2tab,segtab,regtab,flagtab,stack

          public    axsave,bxsave,cxsave,dxsave,bpsave,spsave,sisave
          public    disave,dssave,essave,sssave,cssave,ipsave,_fsave,rstack
          public    regdif,rdflg,totreg,dsiz,noregl,dispb,lbufsiz,lbuffcnt
          public    linebuf,pflag,colpos

          public    qflag,newexec,retsave,user_proc_pdb,headsave,exec_block
          public    com_line,com_fcb1,com_fcb2,com_sssp,com_csip

reg8      db        "ALCLDLBLAHCHDHBH"
reg16     db        "AXCXDXBXSPBPSIDI"
sreg      db        "ESCSSSDS",0,0
siz8      db        "BYWODWQWTB",0,0
; 0
distab    dw        offset dg:addmn,alufromreg
          dw        offset dg:addmn,alufromreg
          dw        offset dg:addmn,alutoreg
          dw        offset dg:addmn,alutoreg
          dw        offset dg:addmn,accimm
          dw        offset dg:addmn,accimm
          dw        offset dg:pushmn,segop
          dw        offset dg:popmn,segop
          dw        offset dg:ormn,alufromreg
          dw        offset dg:ormn,alufromreg
          dw        offset dg:ormn,alutoreg
          dw        offset dg:ormn,alutoreg
          dw        offset dg:ormn,accimm
          dw        offset dg:ormn,accimm
          dw        offset dg:pushmn,segop
          dw        offset dg:popmn,segop
; 10H
          dw        offset dg:adcmn,alufromreg
          dw        offset dg:adcmn,alufromreg
          dw        offset dg:adcmn,alutoreg
          dw        offset dg:adcmn,alutoreg
          dw        offset dg:adcmn,accimm
          dw        offset dg:adcmn,accimm
          dw        offset dg:pushmn,segop
          dw        offset dg:popmn,segop
          dw        offset dg:sbbmn,alufromreg
          dw        offset dg:sbbmn,alufromreg
          dw        offset dg:sbbmn,alutoreg
          dw        offset dg:sbbmn,alutoreg
          dw        offset dg:sbbmn,accimm
          dw        offset dg:sbbmn,accimm
          dw        offset dg:pushmn,segop
          dw        offset dg:popmn,segop
; 20H
          dw        offset dg:andmn,alufromreg
          dw        offset dg:andmn,alufromreg
          dw        offset dg:andmn,alutoreg
          dw        offset dg:andmn,alutoreg
          dw        offset dg:andmn,accimm
          dw        offset dg:andmn,accimm
          dw        offset dg:essegmn,espre
          dw        offset dg:daamn,nooperands
          dw        offset dg:submn,alufromreg
          dw        offset dg:submn,alufromreg
          dw        offset dg:submn,alutoreg
          dw        offset dg:submn,alutoreg
          dw        offset dg:submn,accimm
          dw        offset dg:submn,accimm
          dw        offset dg:cssegmn,cspre
          dw        offset dg:dasmn,nooperands
; 30H
          dw        offset dg:xormn,alufromreg
          dw        offset dg:xormn,alufromreg
          dw        offset dg:xormn,alutoreg
          dw        offset dg:xormn,alutoreg
          dw        offset dg:xormn,accimm
          dw        offset dg:xormn,accimm
          dw        offset dg:sssegmn,sspre
          dw        offset dg:aaamn,nooperands
          dw        offset dg:cmpmn,alufromreg
          dw        offset dg:cmpmn,alufromreg
          dw        offset dg:cmpmn,alutoreg
          dw        offset dg:cmpmn,alutoreg
          dw        offset dg:cmpmn,accimm
          dw        offset dg:cmpmn,accimm
          dw        offset dg:dssegmn,dspre
          dw        offset dg:aasmn,nooperands
; 40H
          dw        offset dg:incmn,regop
          dw        offset dg:incmn,regop
          dw        offset dg:incmn,regop
          dw        offset dg:incmn,regop
          dw        offset dg:incmn,regop
          dw        offset dg:incmn,regop
          dw        offset dg:incmn,regop
          dw        offset dg:incmn,regop
          dw        offset dg:decmn,regop
          dw        offset dg:decmn,regop
          dw        offset dg:decmn,regop
          dw        offset dg:decmn,regop
          dw        offset dg:decmn,regop
          dw        offset dg:decmn,regop
          dw        offset dg:decmn,regop
          dw        offset dg:decmn,regop
; 50H
          dw        offset dg:pushmn,regop
          dw        offset dg:pushmn,regop
          dw        offset dg:pushmn,regop
          dw        offset dg:pushmn,regop
          dw        offset dg:pushmn,regop
          dw        offset dg:pushmn,regop
          dw        offset dg:pushmn,regop
          dw        offset dg:pushmn,regop
          dw        offset dg:popmn,regop
          dw        offset dg:popmn,regop
          dw        offset dg:popmn,regop
          dw        offset dg:popmn,regop
          dw        offset dg:popmn,regop
          dw        offset dg:popmn,regop
          dw        offset dg:popmn,regop
          dw        offset dg:popmn,regop
; 60H
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
; 70H
          dw        offset dg:jomn,shortjmp
          dw        offset dg:jnomn,shortjmp
          dw        offset dg:jcmn,shortjmp
          dw        offset dg:jncmn,shortjmp
          dw        offset dg:jzmn,shortjmp
          dw        offset dg:jnzmn,shortjmp
          dw        offset dg:jbemn,shortjmp
          dw        offset dg:jamn,shortjmp
          dw        offset dg:jsmn,shortjmp
          dw        offset dg:jnsmn,shortjmp
          dw        offset dg:jpemn,shortjmp
          dw        offset dg:jpomn,shortjmp
          dw        offset dg:jlmn,shortjmp
          dw        offset dg:jgemn,shortjmp
          dw        offset dg:jlemn,shortjmp
          dw        offset dg:jgmn,shortjmp
; 80H
          dw        0,immed
          dw        0,immed
          dw        0,immed
          dw        0,signimm
          dw        offset dg:testmn,alufromreg
          dw        offset dg:testmn,alufromreg
          dw        offset dg:xchgmn,alufromreg
          dw        offset dg:xchgmn,alufromreg
          dw        offset dg:movmn,alufromreg
          dw        offset dg:movmn,alufromreg
          dw        offset dg:movmn,alutoreg
          dw        offset dg:movmn,alutoreg
          dw        offset dg:movmn,movsegto
          dw        offset dg:leamn,wordtoalu
          dw        offset dg:movmn,movsegfrom
          dw        offset dg:popmn,getaddr
; 90H
          dw        offset dg:nopmn,nooperands
          dw        offset dg:xchgmn,xchgax
          dw        offset dg:xchgmn,xchgax
          dw        offset dg:xchgmn,xchgax
          dw        offset dg:xchgmn,xchgax
          dw        offset dg:xchgmn,xchgax
          dw        offset dg:xchgmn,xchgax
          dw        offset dg:xchgmn,xchgax
          dw        offset dg:cbwmn,nooperands
          dw        offset dg:cwdmn,nooperands
          dw        offset dg:callmn,longjmp
          dw        offset dg:waitmn,nooperands
          dw        offset dg:pushfmn,nooperands
          dw        offset dg:popfmn,nooperands
          dw        offset dg:sahfmn,nooperands
          dw        offset dg:lahfmn,nooperands
; A0H
          dw        offset dg:movmn,loadacc
          dw        offset dg:movmn,loadacc
          dw        offset dg:movmn,storeacc
          dw        offset dg:movmn,storeacc
          dw        offset dg:movbmn,nooperands
          dw        offset dg:movwmn,nooperands
          dw        offset dg:cmpbmn,nooperands
          dw        offset dg:cmpwmn,nooperands
          dw        offset dg:testmn,accimm
          dw        offset dg:testmn,accimm
          dw        offset dg:stobmn,nooperands
          dw        offset dg:stowmn,nooperands
          dw        offset dg:lodbmn,nooperands
          dw        offset dg:lodwmn,nooperands
          dw        offset dg:scabmn,nooperands
          dw        offset dg:scawmn,nooperands
; B0H
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmb
          dw        offset dg:movmn,regimmw
          dw        offset dg:movmn,regimmw
          dw        offset dg:movmn,regimmw
          dw        offset dg:movmn,regimmw
          dw        offset dg:movmn,regimmw
          dw        offset dg:movmn,regimmw
          dw        offset dg:movmn,regimmw
          dw        offset dg:movmn,regimmw
; C0H
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:retmn,sav16
          dw        offset dg:retmn,nooperands
          dw        offset dg:lesmn,wordtoalu
          dw        offset dg:ldsmn,wordtoalu
          dw        offset dg:movmn,memimm
          dw        offset dg:movmn,memimm
          dw        offset dg:dbmn,savhex
          dw        offset dg:dbmn,savhex
          dw        offset dg:retfmn,sav16
          dw        offset dg:retfmn,nooperands
          dw        offset dg:intmn,int3
          dw        offset dg:intmn,sav8
          dw        offset dg:intomn,nooperands
          dw        offset dg:iretmn,nooperands
; D0H
          dw        0,shift
          dw        0,shift
          dw        0,shiftv
          dw        0,shiftv
          dw        offset dg:aammn,chk10
          dw        offset dg:aadmn,chk10
          dw        offset dg:dbmn,savhex
          dw        offset dg:xlatmn,nooperands
          dw        0,m8087                 ; d8
          dw        0,m8087_d9              ; d9
          dw        0,m8087                 ; da
          dw        0,m8087_db              ; db
          dw        0,m8087                 ; dc
          dw        0,m8087_dd              ; dd
          dw        0,m8087                 ; de
          dw        0,m8087_df              ; df
; E0H
          dw        offset dg:loopnzmn,shortjmp
          dw        offset dg:loopzmn,shortjmp
          dw        offset dg:loopmn,shortjmp
          dw        offset dg:jcxzmn,shortjmp
          dw        offset dg:inmn,infixb
          dw        offset dg:inmn,infixw
          dw        offset dg:outmn,outfixb
          dw        offset dg:outmn,outfixw
          dw        offset dg:callmn,jmpcall
          dw        offset dg:jmpmn,jmpcall
          dw        offset dg:jmpmn,longjmp
          dw        offset dg:jmpmn,shortjmp
          dw        offset dg:inmn,invarb
          dw        offset dg:inmn,invarw
          dw        offset dg:outmn,outvarb
          dw        offset dg:outmn,outvarw
; F0H
          dw        offset dg:lockmn,prefix
          dw        offset dg:dbmn,savhex
          dw        offset dg:repnzmn,prefix
          dw        offset dg:repzmn,prefix
          dw        offset dg:hltmn,nooperands
          dw        offset dg:cmcmn,nooperands
          dw        0,grp1
          dw        0,grp1
          dw        offset dg:clcmn,nooperands
          dw        offset dg:stcmn,nooperands
          dw        offset dg:dimn,nooperands
          dw        offset dg:eimn,nooperands
          dw        offset dg:upmn,nooperands
          dw        offset dg:downmn,nooperands
          dw        0,grp2
          dw        0,grp2

dbmn      db        "D","B"+80H
          db        "D","W"+80H
          db        ";"+80H
addmn     db        "AD","D"+80H
adcmn     db        "AD","C"+80H
submn     db        "SU","B"+80H
sbbmn     db        "SB","B"+80H
xormn     db        "XO","R"+80H
ormn      db        "O","R"+80H
andmn     db        "AN","D"+80H
aaamn     db        "AA","A"+80H
aadmn     db        "AA","D"+80H
aammn     db        "AA","M"+80H
aasmn     db        "AA","S"+80H
callmn    db        "CAL","L"+80H
cbwmn     db        "CB","W"+80H
clcmn     db        "CL","C"+80H
upmn      db        "CL","D"+80H            ; CLD+80H
dimn      db        "CL","I"+80H
cmcmn     db        "CM","C"+80H
cmpbmn    db        "CMPS","B"+80H          ; CMPSB
cmpwmn    db        "CMPS","W"+80H          ; CMPSW+80H
cmpmn     db        "CM","P"+80H
cwdmn     db        "CW","D"+80H
daamn     db        "DA","A"+80H
dasmn     db        "DA","S"+80H
decmn     db        "DE","C"+80H
divmn     db        "DI","V"+80H
escmn     db        "ES","C"+80H
          db        "FXC","H"+80H
          db        "FFRE","E"+80H
          db        "FCOMP","P"+80H
          db        "FCOM","P"+80H
          db        "FCO","M"+80H
          db        "FICOM","P"+80H
          db        "FICO","M"+80H
          db        "FNO","P"+80H
          db        "FCH","S"+80H
          db        "FAB","S"+80H
          db        "FTS","T"+80H
          db        "FXA","M"+80H
          db        "FLDL2","T"+80H
          db        "FLDL2","E"+80H
          db        "FLDLG","2"+80H
          db        "FLDLN","2"+80H
          db        "FLDP","I"+80H
          db        "FLD","1"+80H
          db        "FLD","Z"+80H
          db        "F2XM","1"+80H
          db        "FYL2XP","1"+80H
          db        "FYL2","X"+80H
          db        "FPTA","N"+80H
          db        "FPATA","N"+80H
          db        "FXTRAC","T"+80H
          db        "FDECST","P"+80H
          db        "FINCST","P"+80H
          db        "FPRE","M"+80H
          db        "FSQR","T"+80H
          db        "FRNDIN","T"+80H
          db        "FSCAL","E"+80H
          db        "FINI","T"+80H
          db        "FDIS","I"+80H
          db        "FEN","I"+80H
          db        "FCLE","X"+80H
          db        "FBL","D"+80H
          db        "FBST","P"+80H
          db        "FLDC","W"+80H
          db        "FSTC","W"+80H
          db        "FSTS","W"+80H
          db        "FSTEN","V"+80H
          db        "FLDEN","V"+80H
          db        "FSAV","E"+80H
          db        "FRSTO","R"+80H
          db        "FADD","P"+80H
          db        "FAD","D"+80H
          db        "FIAD","D"+80H
          db        "FSUBR","P"+80H
          db        "FSUB","R"+80H
          db        "FSUB","P"+80H
          db        "FSU","B"+80H
          db        "FISUB","R"+80H
          db        "FISU","B"+80H
          db        "FMUL","P"+80H
          db        "FMU","L"+80H
          db        "FIMU","L"+80H
          db        "FDIVR","P"+80H
          db        "FDIV","R"+80H
          db        "FDIV","P"+80H
          db        "FDI","V"+80H
          db        "FIDIV","R"+80H
          db        "FIDI","V"+80H
          db        "FWAI","T"+80H
          db        "FIL","D"+80H
          db        "FL","D"+80H
          db        "FST","P"+80H
          db        "FS","T"+80H
          db        "FIST","P"+80H
          db        "FIS","T"+80H
hltmn     db        "HL","T"+80H
idivmn    db        "IDI","V"+80H
imulmn    db        "IMU","L"+80H
incmn     db        "IN","C"+80H
intomn    db        "INT","O"+80H
intmn     db        "IN","T"+80H
inmn      db        "I","N"+80H             ; IN
iretmn    db        "IRE","T"+80H
          db        "JNB","E"+80H
          db        "JA","E"+80H
jamn      db        "J","A"+80H
jcxzmn    db        "JCX","Z"+80H
jncmn     db        "JN","B"+80H
jbemn     db        "JB","E"+80H
jcmn      db        "J","B"+80H
          db        "JN","C"+80H
          db        "J","C"+80H
          db        "JNA","E"+80H
          db        "JN","A"+80H
jzmn      db        "J","Z"+80H
          db        "J","E"+80H
jgemn     db        "JG","E"+80H
jgmn      db        "J","G"+80H
          db        "JNL","E"+80H
          db        "JN","L"+80H
jlemn     db        "JL","E"+80H
jlmn      db        "J","L"+80H
          db        "JNG","E"+80H
          db        "JN","G"+80H
jmpmn     db        "JM","P"+80H
jnzmn     db        "JN","Z"+80H
          db        "JN","E"+80H
jpemn     db        "JP","E"+80H
jpomn     db        "JP","O"+80H
          db        "JN","P"+80H
jnsmn     db        "JN","S"+80H
jnomn     db        "JN","O"+80H
jomn      db        "J","O"+80H
jsmn      db        "J","S"+80H
          db        "J","P"+80H
lahfmn    db        "LAH","F"+80H
ldsmn     db        "LD","S"+80H
leamn     db        "LE","A"+80H
lesmn     db        "LE","S"+80H
lockmn    db        "LOC","K"+80H
lodbmn    db        "LODS","B"+80H          ; LODSB
lodwmn    db        "LODS","W"+80H          ; LODSW+80H
loopnzmn  db        "LOOPN","Z"+80H
loopzmn   db        "LOOP","Z"+80H
          db        "LOOPN","E"+80H
          db        "LOOP","E"+80H
loopmn    db        "LOO","P"+80H
movbmn    db        "MOVS","B"+80H          ; MOVSB
movwmn    db        "MOVS","W"+80H          ; MOVSW+80H
movmn     db        "MO","V"+80H
mulmn     db        "MU","L"+80H
negmn     db        "NE","G"+80H
nopmn     db        "NO","P"+80H
notmn     db        "NO","T"+80H
outmn     db        "OU","T"+80H            ; OUT
popfmn    db        "POP","F"+80H
popmn     db        "PO","P"+80H
pushfmn   db        "PUSH","F"+80H
pushmn    db        "PUS","H"+80H
rclmn     db        "RC","L"+80H
rcrmn     db        "RC","R"+80H
repzmn    db        "REP","Z"+80H
repnzmn   db        "REPN","Z"+80H
          db        "REP","E"+80H
          db        "REPN","E"+80H
          db        "RE","P"+80H
retfmn    db        "RET","F"+80H
retmn     db        "RE","T"+80H
rolmn     db        "RO","L"+80H
rormn     db        "RO","R"+80H
sahfmn    db        "SAH","F"+80H
sarmn     db        "SA","R"+80H
scabmn    db        "SCAS","B"+80H          ; SCASB
scawmn    db        "SCAS","W"+80H          ; SCASW+80H
shlmn     db        "SH","L"+80H
shrmn     db        "SH","R"+80H
stcmn     db        "ST","C"+80H
downmn    db        "ST","D"+80H            ; STD
eimn      db        "ST","I"+80H            ; STI
stobmn    db        "STOS","B"+80H          ; STOSB
stowmn    db        "STOS","W"+80H          ; STOSW+80H
testmn    db        "TES","T"+80H
waitmn    db        "WAI","T"+80H
xchgmn    db        "XCH","G"+80H
xlatmn    db        "XLA","T"+80H
essegmn   db        "ES",":"+80H
cssegmn   db        "CS",":"+80H
sssegmn   db        "SS",":"+80H
dssegmn   db        "DS",":"+80H
badmn     db        "??","?"+80H

m8087_tab db "ADD$MUL$COM$COMP$SUB$SUBR$DIV$DIVR$"
fi_tab    db "F$FI$F$FI$"
size_tab  db "DWORD PTR $DWORD PTR $QWORD PTR $WORD PTR $"
          db "BYTE PTR $TBYTE PTR $"

md9_tab   db "LD$@$ST$STP$LDENV$LDCW$STENV$STCW$"
md9_tab2  db "CHS$ABS$@$@$TST$XAM$@$@$LD1$LDL2T$LDL2E$"
          db "LDPI$LDLG2$LDLN2$LDZ$@$2XM1$YL2X$PTAN$PATAN$XTRACT$"
          db "@$DECSTP$INCSTP$PREM$YL2XP1$SQRT$@$RNDINT$SCALE$@$@$"

mdb_tab   db  "ILD$@$IST$ISTP$@$LD$@$STP$"
mdb_tab2  db  "ENI$DISI$CLEX$INIT$"

mdd_tab   db "LD$@$ST$STP$RSTOR$@$SAVE$STSW$"
mdd_tab2  db "FREE$XCH$ST$STP$"

mdf_tab   db "ILD$@$IST$ISTP$BLD$ILD$BSTP$ISTP$"

optab     db        11111111b               ; db
          dw        db_oper
          db        11111111b               ; dw
          dw        dw_oper
          db        11111111b               ; comment
          dw        assemloop
          db        0 * 8                   ; add
          dw        group2
          db        2 * 8                   ; adc
          dw        group2
          db        5 * 8                   ; sub
          dw        group2
          db        3 * 8                   ; sbb
          dw        group2
          db        6 * 8                   ; xor
          dw        group2
          db        1 * 8                   ; or
          dw        group2
          db        4 * 8                   ; and
          dw        group2
          db        00110111b               ; aaa
          dw        no_oper
          db        11010101b               ; aad
          dw        aa_oper
          db        11010100b               ; aam
          dw        aa_oper
          db        00111111b               ; aas
          dw        no_oper
          db        2 * 8                   ; call
          dw        call_oper
          db        10011000b               ; cbw
          dw        no_oper
          db        11111000b               ; clc
          dw        no_oper
          db        11111100b               ; cld
          dw        no_oper
          db        11111010b               ; dim
          dw        no_oper
          db        11110101b               ; cmc
          dw        no_oper
          db        10100110b               ; cmpb
          dw        no_oper
          db        10100111b               ; cmpw
          dw        no_oper
          db        7 * 8                   ; cmp
          dw        group2
          db        10011001b               ; cwd
          dw        no_oper
          db        00100111b               ; daa
          dw        no_oper
          db        00101111b               ; das
          dw        no_oper
          db        1 * 8                   ; dec
          dw        dcinc_oper
          db        6 * 8                   ; div
          dw        group1
          db        11011000b               ; esc
          dw        esc_oper
          db        00001001b               ; fxch
          dw        fgroupp
          db        00101000b               ; ffree
          dw        fgroupp
          db        11011001b               ; fcompp
          dw        fde_oper
          db        00000011b               ; fcomp
          dw        fgroupx                 ; exception to normal p instructions
          db        00000010b               ; fcom
          dw        fgroupx
          db        00010011b               ; ficomp
          dw        fgroupz
          db        00010010b               ; ficom
          dw        fgroupz
          db        11010000b               ; fnop
          dw        fd9_oper
          db        11100000b               ; fchs
          dw        fd9_oper
          db        11100001b               ; fabs
          dw        fd9_oper
          db        11100100b               ; ftst
          dw        fd9_oper
          db        11100101b               ; fxam
          dw        fd9_oper
          db        11101001b               ; fldl2t
          dw        fd9_oper
          db        11101010b               ; fldl2e
          dw        fd9_oper
          db        11101100b               ; fldlg2
          dw        fd9_oper
          db        11101101b               ; fldln2
          dw        fd9_oper
          db        11101011b               ; fldpi
          dw        fd9_oper
          db        11101000b               ; fld1
          dw        fd9_oper
          db        11101110b               ; fldz
          dw        fd9_oper
          db        11110000b               ; f2xm1
          dw        fd9_oper
          db        11111001b               ; fyl2xp1
          dw        fd9_oper
          db        11110001b               ; fyl2x
          dw        fd9_oper
          db        11110010b               ; fptan
          dw        fd9_oper
          db        11110011b               ; fpatan
          dw        fd9_oper
          db        11110100b               ; fxtract
          dw        fd9_oper
          db        11110110b               ; fdecstp
          dw        fd9_oper
          db        11110111b               ; fincstp
          dw        fd9_oper
          db        11111000b               ; fprem
          dw        fd9_oper
          db        11111010b               ; fsqrt
          dw        fd9_oper
          db        11111100b               ; frndint
          dw        fd9_oper
          db        11111101b               ; fscale
          dw        fd9_oper
          db        11100011b               ; finit
          dw        fdb_oper
          db        11100001b               ; fdisi
          dw        fdb_oper
          db        11100000b               ; feni
          dw        fdb_oper
          db        11100010b               ; fclex
          dw        fdb_oper
          db        00111100b               ; fbld
          dw        fgroupb
          db        00111110b               ; fbstp
          dw        fgroupb
          db        00001101b               ; fldcw
          dw        fgroup3w
          db        00001111b               ; fstcw
          dw        fgroup3w
          db        00101111b               ; fstsw
          dw        fgroup3w
          db        00001110b               ; fstenv
          dw        fgroup3
          db        00001100b               ; fldenv
          dw        fgroup3
          db        00101110b               ; _fsave
          dw        fgroup3
          db        00101100b               ; frstor
          dw        fgroup3
          db        00110000b               ; faddp
          dw        fgroupp
          db        00000000b               ; fadd
          dw        fgroup
          db        00010000b               ; fiadd
          dw        fgroupz
          db        00110100b               ; fsubrp
          dw        fgroupp
          db        00000101b               ; fsubr
          dw        fgroupds
          db        00110101b               ; fsubp
          dw        fgroupp
          db        00000100b               ; fsub
          dw        fgroupds
          db        00010101b               ; fisubr
          dw        fgroupz
          db        00010100b               ; fisub
          dw        fgroupz
          db        00110001b               ; fmulp
          dw        fgroupp
          db        00000001b               ; fmul
          dw        fgroup
          db        00010001b               ; fimul
          dw        fgroupz
          db        00110110b               ; fdivrp
          dw        fgroupp
          db        00000111b               ; fdivr
          dw        fgroupds
          db        00110111b               ; fdivp
          dw        fgroupp
          db        00000110b               ; fdiv
          dw        fgroupds
          db        00010111b               ; fidivr
          dw        fgroupz
          db        00010110b               ; fidiv
          dw        fgroupz
          db        10011011b               ; fwait
          dw        no_oper
          db        00011000b               ; fild
          dw        fgroupz
          db        00001000b               ; fld
          dw        fgroupx
          db        00001011b               ; fstp
          dw        fgroupx
          db        00101010b               ; fst
          dw        fgroupx
          db        00011011b               ; fistp
          dw        fgroupz
          db        00011010b               ; fist
          dw        fgroupz
          db        11110100b               ; hlt
          dw        no_oper
          db        7 * 8                   ; idiv
          dw        group1
          db        5 * 8                   ; imul
          dw        group1
          db        0 * 8                   ; inc
          dw        dcinc_oper
          db        11001110b               ; into
          dw        no_oper
          db        11001100b               ; intm
          dw        int_oper
          db        11101100b               ; in
          dw        in_oper
          db        11001111b               ; iret
          dw        no_oper
          db        01110111b               ; jnbe
          dw        disp8_oper
          db        01110011b               ; jae
          dw        disp8_oper
          db        01110111b               ; ja
          dw        disp8_oper
          db        11100011b               ; jcxz
          dw        disp8_oper
          db        01110011b               ; jnb
          dw        disp8_oper
          db        01110110b               ; jbe
          dw        disp8_oper
          db        01110010b               ; jb
          dw        disp8_oper
          db        01110011b               ; jnc
          dw        disp8_oper
          db        01110010b               ; jc
          dw        disp8_oper
          db        01110010b               ; jnae
          dw        disp8_oper
          db        01110110b               ; jna
          dw        disp8_oper
          db        01110100b               ; jz
          dw        disp8_oper
          db        01110100b               ; je
          dw        disp8_oper
          db        01111101b               ; jge
          dw        disp8_oper
          db        01111111b               ; jg
          dw        disp8_oper
          db        01111111b               ; jnle
          dw        disp8_oper
          db        01111101b               ; jnl
          dw        disp8_oper
          db        01111110b               ; jle
          dw        disp8_oper
          db        01111100b               ; jl
          dw        disp8_oper
          db        01111100b               ; jnge
          dw        disp8_oper
          db        01111110b               ; jng
          dw        disp8_oper
          db        4 * 8                   ; jmp
          dw        jmp_oper
          db        01110101b               ; jnz
          dw        disp8_oper
          db        01110101b               ; jne
          dw        disp8_oper
          db        01111010b               ; jpe
          dw        disp8_oper
          db        01111011b               ; jpo
          dw        disp8_oper
          db        01111011b               ; jnp
          dw        disp8_oper
          db        01111001b               ; jns
          dw        disp8_oper
          db        01110001b               ; jno
          dw        disp8_oper
          db        01110000b               ; jo
          dw        disp8_oper
          db        01111000b               ; js
          dw        disp8_oper
          db        01111010b               ; jp
          dw        disp8_oper
          db        10011111b               ; lahf
          dw        no_oper
          db        11000101b               ; lds
          dw        l_oper
          db        10001101b               ; lea
          dw        l_oper
          db        11000100b               ; les
          dw        l_oper
          db        11110000b               ; lock
          dw        no_oper
          db        10101100b               ; lodb
          dw        no_oper
          db        10101101b               ; lodw
          dw        no_oper
          db        11100000b               ; loopnz
          dw        disp8_oper
          db        11100001b               ; loopz
          dw        disp8_oper
          db        11100000b               ; loopne
          dw        disp8_oper
          db        11100001b               ; loope
          dw        disp8_oper
          db        11100010b               ; loop
          dw        disp8_oper
          db        10100100b               ; movb
          dw        no_oper
          db        10100101b               ; movw
          dw        no_oper
          db        11000110b               ; mov
          dw        mov_oper
          db        4 * 8                   ; mul
          dw        group1
          db        3 * 8                   ; neg
          dw        group1
          db        10010000b               ; nop
          dw        no_oper
          db        2 * 8                   ; not
          dw        group1
          db        11101110b               ; out
          dw        out_oper
          db        10011101b               ; popf
          dw        no_oper
          db        0 * 8                   ; pop
          dw        pop_oper
          db        10011100b               ; pushf
          dw        no_oper
          db        6 * 8                   ; push
          dw        push_oper
          db        2 * 8                   ; rcl
          dw        rotop
          db        3 * 8                   ; rcr
          dw        rotop
          db        11110011b               ; repz
          dw        no_oper
          db        11110010b               ; repnz
          dw        no_oper
          db        11110011b               ; repe
          dw        no_oper
          db        11110010b               ; repne
          dw        no_oper
          db        11110011b               ; rep
          dw        no_oper
          db        11001011b               ; retf
          dw        get_data16
          db        11000011b               ; ret
          dw        get_data16
          db        0 * 8                   ; rol
          dw        rotop
          db        1 * 8                   ; ror
          dw        rotop
          db        10011110b               ; sahf
          dw        no_oper
          db        7 * 8                   ; sar
          dw        rotop
          db        10101110b               ; scab
          dw        no_oper
          db        10101111b               ; scaw
          dw        no_oper
          db        4 * 8                   ; shl
          dw        rotop
          db        5 * 8                   ; shr
          dw        rotop
          db        11111001b               ; stc
          dw        no_oper
          db        11111101b               ; std
          dw        no_oper
          db        11111011b               ; ei
          dw        no_oper
          db        10101010b               ; stob
          dw        no_oper
          db        10101011b               ; stow
          dw        no_oper
          db        11110110b               ; test
          dw        tst_oper
          db        10011011b               ; wait
          dw        no_oper
          db        10000110b               ; xchg
          dw        ex_oper
          db        11010111b               ; xlat
          dw        no_oper
          db        00100110b               ; esseg
          dw        no_oper
          db        00101110b               ; csseg
          dw        no_oper
          db        00110110b               ; ssseg
          dw        no_oper
          db        00111110b               ; dsseg
          dw        no_oper

zzopcode label  byte
maxop   = (zzopcode-optab)/3

shftab    dw               offset dg:rolmn,offset dg:rormn,offset dg:rclmn
          dw               offset dg:rcrmn,offset dg:shlmn,offset dg:shrmn
          dw               offset dg:badmn,offset dg:sarmn

immtab    dw        offset dg:addmn,offset dg:ormn,offset dg:adcmn
          dw        offset dg:sbbmn,offset dg:andmn,offset dg:submn
          dw        offset dg:xormn,offset dg:cmpmn

grp1tab   dw        offset dg:testmn,offset dg:badmn,offset dg:notmn
          dw        offset dg:negmn,offset dg:mulmn,offset dg:imulmn
          dw        offset dg:divmn,offset dg:idivmn

grp2tab   dw        offset dg:incmn,offset dg:decmn,offset dg:callmn
          dw        offset dg:callmn,offset dg:jmpmn,offset dg:jmpmn
          dw        offset dg:pushmn,offset dg:badmn

segtab    dw        offset dg:essave,offset dg:cssave,offset dg:sssave
          dw        offset dg:dssave

regtab    db        "AXBXCXDXSPBPSIDIDSESSSCSIPPC"

; Flags are ordered to correspond with the bits of the flag
; register, most significant bit first, zero if bit is not
; a flag. First 16 entries are for bit set, second 16 for
; bit reset.

flagtab   dw        0
          dw        0
          dw        0
          dw        0
          db        "OV"
          db        "DN"
          db        "EI"                    ; "STI"
          dw        0
          db        "NG"
          db        "ZR"
          dw        0
          db        "AC"
          dw        0
          db        "PE"
          dw        0
          db        "CY"
          dw        0
          dw        0
          dw        0
          dw        0
          db        "NV"
          db        "UP"                    ; "CLD"
          db        "DI"
          dw        0
          db        "PL"
          db        "NZ"
          dw        0
          db        "NA"
          dw        0
          db        "PO"
          dw        0
          db        "NC"

          db        80h dup(?)
stack     label     byte


; Register save area

axsave    dw        0
bxsave    dw        0
cxsave    dw        0
dxsave    dw        0
spsave    dw        5ah
bpsave    dw        0
sisave    dw        0
disave    dw        0
dssave    dw        0
essave    dw        0
rstack    label     word                    ; Stack set here so registers can be saved by pushing
sssave    dw        0
cssave    dw        0
ipsave    dw        100h
_fsave   dw      0

regdif    equ        axsave-regtab

; ram area.

rdflg     db        read
totreg    db        13
dsiz      db        0fh
noregl    db        8
dispb     dw        128

lbufsiz         db      buflen
lbuffcnt        db      0
linebuf   db        0dh
          db        buflen dup (?)
pflag     db        0
colpos    db        0

qflag     db        0
newexec   db        0
retsave   dw        ?

user_proc_pdb dw ?

headsave dw     ?

exec_block label byte
          dw        0
com_line label  dword
          dw        80h
          dw        ?
com_fcb1 label  dword
          dw        fcb
          dw        ?
com_fcb2 label  dword
          dw        fcb + 10h
          dw        ?
com_sssp dd     ?
com_csip dd     ?

const     ends
          end
