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

; Code for the UASSEMble command in the debugger

          include   debequ.inc
          include   dossym.inc

code      segment   public byte 'code'
code      ends

const     segment   public byte

          extrn     synerr:byte
          extrn     nseg:word,sisave:word,bpsave:word,disave:word
          extrn     bxsave:word,dssave:word,essave:word,cssave:word,ipsave:word
          extrn     sssave:word,cxsave:word,spsave:word,_fsave:word
          extrn     distab:word,shftab:word,immtab:word,grp1tab:word,grp2tab:word
          extrn     dbmn:byte,escmn:byte,dispb:word,stack:byte,reg8:byte
          extrn     reg16:byte,sreg:byte,siz8:byte,segtab:word,m8087_tab:byte
          extrn     fi_tab:byte,size_tab:byte,md9_tab:byte,md9_tab2:byte
          extrn     mdb_tab:byte,mdb_tab2:byte,mdd_tab:byte,mdd_tab2:byte
          extrn     mdf_tab:byte

const   ends

data    segment public byte

          extrn     disadd:byte,discnt:word,bytcnt:byte,temp:byte,aword:byte
          extrn     midfld:byte,mode:byte,regmem:byte,opcode:word,opbuf:byte
          extrn     index:word

data    ends

dg      group   code,const,data


code    segment public byte 'code'
assume  cs:dg,ds:dg,es:dg,ss:dg


          public    unassem
          public    disasln,memimm,jmpcall,signimm,alufromreg,wordtoalu
          public    grp2,prefix,outvarw,grp1,sspre,movsegto,dspre,shift
          public    espre,immed,cspre,outvarb,chk10,accimm,int3,invarb
          public    movsegfrom,loadacc,outfixb,xchgax,regimmw,shortjmp
          public    sav8,m8087,m8087_db,m8087_df,m8087_d9,m8087_dd
          public    sav16,savhex,infixw,regimmb,outfixw,shiftv,longjmp
          public    invarw,storeacc,infixb,nooperands,alutoreg
          public    segop,regop,getaddr

          extrn     crlf:near,printmes:near,blank:near,tab:near,_out:near
          extrn     hex:near,default:near,outsi:near,outdi:near

unassem:  mov       bp,[cssave]         ; Default code segment
          mov       di,offset dg:disadd ; Default address
          mov       cx,dispb            ; Default length
          shr       cx,1
          shr       cx,1
          call      default
          mov       word ptr [disadd],dx
          mov       word ptr [disadd+2],ax
          mov       word ptr [discnt],cx
dislp:    call      disasln             ; Disassemble one line
          call      crlf
          test      [discnt],-1         ; See if we've used up the range
          jnz       dislp
          ret
gotdis:   push      ds                  ; RE-GET LAST BYTE
          push      si
          lds       si,dword ptr [disadd]
          mov       al,[si-1]
          pop       si
          pop       ds
          ret
getdis:   push      ds
          lds       si,dword ptr [disadd]
          lodsb                         ; Get the next byte of code
          pop       ds
          mov       word ptr [disadd],si
          push      ax
          call      hex                 ; Display each code byte
          mov       si,[discnt]
          or        si,si               ; Check if range exhausted
          jz        endrng              ; If so, don't wrap around
          dec       si                  ; Count off the bytes
          mov       [discnt],si
endrng:   inc       byte ptr[bytcnt]    ; Keep track of no. of bytes per line
          pop       ax
          ret
dspre:    inc       byte ptr [nseg+1]
sspre:    inc       byte ptr [nseg+1]
cspre:    inc       byte ptr [nseg+1]
espre:    inc       byte ptr [nseg+1]
prefix:   pop       bx                  ; Dump off return address
          call      finln
          call      crlf
disasln:  push      ds
          lds       si,dword ptr [disadd]
          call      outsi               ; Show disassembly address
          pop       ds
          call      blank
disasln1: mov       byte ptr [bytcnt],0 ; Count of code bytes per line
          mov       di,offset dg:opbuf  ; Point to operand buffer
          mov       al,' '
          mov       cx,opbuflen-1       ; Don't do last byte which has end marker
          rep       stosb               ; Initialize operand buffer to blanks
          mov       byte ptr [di],' '+80h
          call      getdis              ; Get opcode
          mov       ah,0
          mov       bx,ax
          and       al,1                ; Mask to "W" bit
          mov       [aword],al
          mov       al,bl               ; Restore opcode
          shl       bx,1
          shl       bx,1                ; Multiply opcode by 4
          add       bx,offset dg:distab
          mov       dx,[bx]             ; Get pointer to mnemonic from table
          mov       [opcode],dx         ; Save it until line is complete
          mov       di,offset dg:opbuf  ; Initialize for opcode routines
          call      word ptr [bx+2]     ; Dispatch to opcode routine
finln:
          mov       si,offset dg:disadd
          mov       ah,[bytcnt]         ; See how many bytes in this instruction
          add       ah,ah               ; Each uses two characters
          mov       al,14               ; Amount of space we want to use
          sub       al,ah               ; See how many fill characters needed
          cbw
          xchg      cx,ax               ; Parameter for TAB needed in CX
          call      tab
          mov       si,[opcode]
          or        si,si               ; MAKE SURE THERE IS SOMETHING TO PRINT
          jz        noopc
          call      printmes            ; Print opcode mnemonic
          mov       al,9
          call      _out                ; and a tab
noopc:    mov       si,offset dg:opbuf
          jmp       printmes            ; and the operand buffer
getmode:  call      getdis              ; Get the address mode byte
          mov       ah,al
          and       al,7                ; Mask to "r/m" field
          mov       [regmem],al
          shr       ah,1
          shr       ah,1
          shr       ah,1
          mov       al,ah
          and       al,7                ; Mask to center 3-bit field
          mov       [midfld],al
          shr       ah,1
          shr       ah,1
          shr       ah,1
          mov       [mode],ah           ; Leaving 2-bit "MOD" field
          ret
immed:    mov       bx,offset dg:immtab
          call      getmne
finimm:   call      testreg
          jmp short imm
memimm:   call      getmode
          jmp short finimm
accimm:   xor       al,al
imm1:     call      savreg
imm:      mov       al,','
          stosb
          test      byte ptr [aword],-1
          jnz       sav16
sav8:     call      getdis
          jmp short savhex
longjmp:  push      di
          mov       di,offset dg:temp
          call      sav16
          pop       di
          call      sav16
          mov       al,':'
          stosb
          mov       si,offset dg:temp
          mov       cx,4
movdig:   lodsb
          stosb
          loop      movdig
          ret
sav16:    call      getdis              ; Get low byte
          mov       dl,al
          call      getdis              ; Get high byte
          mov       dh,al
          call      savhex              ; Convert and store high byte
          mov       al,dl
savhex:   mov       ah,al
          shr       al,1
          shr       al,1
          shr       al,1
          shr       al,1
          call      savdig
          mov       al,ah
savdig:   and       al,0fh
          add       al,90h
          daa
          adc       al,40h
          daa
          stosb
          ret
chk10:    call      getdis
          cmp       al,10
          jnz       savhex
          ret
signimm:  mov       bx,offset dg:immtab
          call      getmne
          call      testreg
          mov       al,','
          stosb
savd8:    call      getdis              ; Get signed 8-bit number
          cbw
          mov       dx,ax               ; Save true 16-bit value in DX
          mov       ah,al
          mov       al,'+'
          or        ah,ah
          jns       positiv             ; OK if positive
          mov       al,'-'
          neg       ah                  ; Get magnitude if negative
positiv:  stosb
          mov       al,ah
          jmp short savhex
alufromreg:
          call      getaddr
          mov       al,','
          stosb
regfld:   mov       al,[midfld]
savreg:   mov       si,offset dg:reg8
          cmp       byte ptr [aword],1
          jne       fndreg
savreg16: mov       si,offset dg:reg16
fndreg:   cbw
          add       si,ax
          add       si,ax
          movsw
          ret
segop:    shr       al,1
          shr       al,1
          shr       al,1
savseg:   and       al,3
          mov       si,offset dg:sreg
          jmp short fndreg
regop:    and       al,7
          jmp short savreg16
movsegto: mov       byte ptr [aword],1
          call      getaddr
          mov       al,','
          stosb
          mov       al,[midfld]
          jmp       short savseg
movsegfrom:
          call      getmode
          call      savseg
          mov       byte ptr [aword],1
          jmp short memop2
getaddr:  call      getmode
          jmp short addrmod
wordtoalu:
          mov       byte ptr [aword],1
alutoreg: call      getmode
          call      regfld
memop2:   mov       al,','
          stosb
addrmod:  cmp       byte ptr [mode],3
          mov       al,[regmem]
          je        savreg
          xor       bx,bx
          mov       byte ptr [nseg],3
          mov       byte ptr [di],'['
          inc       di
          cmp       al,6
          jne       nodrct
          cmp       byte ptr [mode],0
          je        direct              ; Mode=0 and R/M=6 means direct addr.
nodrct:   mov       dl,al
          cmp       al,1
          jbe       usebx
          cmp       al,7
          je        usebx
          cmp       al,3
          jbe       usebp
          cmp       al,6
          jne       chkpls
usebp:    mov       bx,[bpsave]
          mov       byte ptr [nseg],2   ; Change default to Stack Segment
          mov       ax,bpreg
savbase:  stosw
chkpls:   cmp       dl,4
          jae       noplus
          mov       al,'+'
          stosb
noplus:   cmp       dl,6
          jae       domode              ; No index register
          and       dl,1                ; Even for SI, odd for DI
          jz        usesi
          add       bx,[disave]
          mov       ax,direg
savindx:  stosw
domode:   mov       al,[mode]
          or        al,al
          jz        closadd             ; If no displacement, then done
          cmp       al,2
          jz        adddir
          call      savd8               ; Signed 8-bit displacement
addclos:  add       bx,dx
closadd:  mov       al,']'
          stosb
          mov       [index],bx
nooperands:
          ret
adddir:   mov       al,'+'
          stosb
direct:   call      sav16
          jmp short addclos
usebx:    mov       bx,[bxsave]
          mov       ax,bxreg
          jmp short savbase
usesi:    add       bx,[sisave]
          mov       ax,sireg
          jmp short savindx
shortjmp: call      getdis
          cbw
          add       ax,word ptr [disadd]
          xchg      dx,ax
savjmp:   mov       al,dh
          call      savhex
          mov       al,dl
          jmp       savhex
jmpcall:  call      getdis
          mov       dl,al
          call      getdis
          mov       dh,al
          add       dx,word ptr [disadd]
          jmp short savjmp
xchgax:   and       al,7
          call      savreg16
          mov       al,','
          stosb
          xor       al,al
          jmp       savreg16
loadacc:  xor       al,al
          call      savreg
          mov       al,','
          stosb
memdir:   mov       al,'['
          stosb
          xor       bx,bx
          mov       byte ptr [nseg],3
          jmp       direct
storeacc: call      memdir
          mov       al,','
          stosb
          xor       al,al
          jmp       savreg
regimmb:  mov       byte ptr [aword],0
          jmp short regimm
regimmw:  mov       byte ptr [aword],1
regimm:   and       al,7
          jmp       imm1
int3:     mov       byte ptr [di],"3"
          ret
;
;  8087 instructions whose first byte is 0dfh
;
m8087_df: call      get64f
          jz        isdd3
          mov       si,offset dg:mdf_tab
          jmp       nodb3
;
;  8087 instructions whose first byte is 0ddh
;
m8087_dd: call      get64f
          jz        isdd3
          mov       si,offset dg:mdd_tab
          jmp       nod93
isdd3:    mov       al,dl
          test      al,100b
          jz        issti
          jmp       esc0
issti:    and       al,11b
          mov       si,offset dg:mdd_tab2
          mov       cl,al
          call      movbyt
          jmp       putrst
;
;  8087 instructions whose first byte is 0dbh
;
m8087_db: call      get64f
          jz        isdb3
          mov       si,offset dg:mdb_tab
nodb3:    call      putop
          call      putsize
          jmp       addrmod
isdb3:    mov       al,dl
          test      al,100b
          jnz       isdbig
esc0v:    jmp       esc0
isdbig:   call      gotdis
          and       al,11111b
          cmp       al,4
          jae       esc0v
          mov       si,offset dg:mdb_tab2
          jmp       dobig
;
;  8087 instructions whose first byte is 0d9h
;
m8087_d9: call      get64f
          jz        isd93
          mov       si,offset dg:md9_tab
nod93:    call      putop
          and       al,111b
          cmp       al,3
          ja        nosho
          mov       al,dl
          call      putsize
nosho:    jmp       addrmod
isd93:    mov       al,dl
          test      al,100b
          jnz       isd9big
          and       al,111b
          or        al,al
          jnz       notfld
          mov       ax,'DL'
          stosw
          jmp       short putrst
notfld:   cmp       al,1
          jnz       notfxch
          mov       ax,'CX'
          stosw
          mov       al,'H'
          jmp       short putrst1
notfxch:  cmp       al,3
          jnz       notfstp
          mov       ax,'TS'
          stosw
          mov       al,'P'
putrst1:  stosb
putrst:   mov       al,9
          stosb
          jmp       putst0
notfstp:  call      gotdis
          cmp       al,11010000b        ; check for fnop
          jz        gotfnop
          jmp       esc0
gotfnop:  mov       ax,'ON'
          stosw
          mov       al,'P'
          stosb
          ret
isd9big:  call      gotdis              ; GET THE MODE BYTE
          mov       si,offset dg:md9_tab2
dobig:    and       al,11111b
          mov       cl,al
          jmp       movbyt
;
; entry point for the remaining 8087 instructions
;
m8087:    call      get64
          call      putfi               ; PUT FIRST PART OF OPCODE
          mov       al,dl
          cmp       byte ptr [mode],11b ; CHECK FOR REGISTER MODE
          jz        modeis3
          call      putmn               ; PUT MIDDLE PART OF OPCODE
no3:      mov       al,9                ; OUTPUT A TAB
          stosb
          mov       al,dl
          call      putsize             ; OUTPUT THE OPERAND SIZE
          jmp       addrmod
modeis3:  test      al,100000b          ; D BIT SET?
          jz        mput                ; NOPE...
          test      al,000100b          ; FDIV OR FSUB?
          jz        mput                ; NOPE...
          xor       al,1                ; REVERSE SENSE OF R
          mov       dl,al               ; SAVE CHANGE
mput:     call      putmn               ; PUT MIDDLE PART OF OPCODE
          mov       al,dl
          test      al,010000b
          jz        nopsh
          mov       al,'P'
          stosb
nopsh:    mov       al,9
          stosb
          mov       al,dl
          and       al,00000111b
          cmp       al,2                ; FCOM
          jz        putst0
          cmp       al,3                ; FCOMP
          jz        putst0
          mov       al,dl
          test      al,100000b
          jz        putstst0
;
; output 8087 registers in the form st(n),st
;
putst0st: call      putst0
          mov       al,','
iscomp:   stosb
putst:    mov       ax,'TS'
          stosw
          ret
;
; output 8087 registers in the form st,st(n)
;
putstst0: call      putst
          mov       al,','
          stosb
putst0:   call      putst
          mov       al,'('
          stosb
          mov       al,[regmem]
          add       al,'0'
          stosb
          mov       al,')'
          stosb
          ret
;
; output an 8087 mnemonic
;
putmn:    mov       si,offset dg:m8087_tab
          mov       cl,al
          and       cl,00000111b
          jmp short movbyt
;
; output either 'FI' or 'F' for first byte of opcode
;
putfi:    mov       si,offset dg:fi_tab
          jmp short putfi2
;
; output size (dword, tbyte, etc.)
;
putsize:  mov       si,offset dg:size_tab
putfi2:   cmp       byte ptr [mode],11b ; check if 8087 register
          jnz       putfi3
          and       al,111000b          ; LOOK FOR INVALID FORM OF 0DAH OPERANDS
          cmp       al,010000b
          jz        esc0pj
          mov       al,dl
          cmp       al,110011b          ; FCOMPP
          jnz       gofi
          cmp       byte ptr [regmem],1
          jz        gofi
esc0pj:   jmp       esc0p
gofi:     xor       cl,cl
          jmp short movbyt
;
;  Look for qword
;
putfi3:   cmp       al,111101b
          jz        gotqu
          cmp       al,111111b
          jnz       notqu
gotqu:    mov       cl,2
          jmp short movbyt
;
;  look for tbyte
;
notqu:    cmp       al,011101b
          jz        gottb
          cmp       al,111100b
          jz        gottb
          cmp       al,111110b
          jz        gottb
          cmp       al,011111b
          jnz       nottb
gottb:    mov       cl,5
          jmp short movbyt
nottb:    mov       cl,4
          shr       al,cl
          mov       cl,al
;
; SI POINTS TO A TABLE OF TEXT SEPARATED BY "$"
; CL = WHICH ELEMENT IN THE TABLE YOU WISH TO COPY TO [DI]
;
movbyt:   push      ax
          inc       cl
movbyt1:  dec       cl
          jz        movbyt3
movbyt2:  lodsb
          cmp       al,'$'
          jz        movbyt1
          jmp       movbyt2
movbyt3:  lodsb
          cmp       al,'$'
          jz        movbyt5
          cmp       al,'@'              ; this means reserved opcode
          jnz       movbyt4
          pop       ax
          jmp       short esc0p         ; go do an escape command
movbyt4:  stosb
          jmp       movbyt3
movbyt5:  pop       ax
          ret

putop:    and       al,111b
          mov       cl,al
          call      movbyt
          mov       al,9
          stosb
          mov       al,dl
          ret

get64f:   call      get64
          mov       al,'F'
          stosb
          cmp       byte ptr [mode],3
          mov       al,dl
          ret
get64:    and       al,7
          mov       dl,al
          call      getmode
          shl       dl,1
          shl       dl,1
          shl       dl,1
          or        al,dl
          mov       dl,al               ; SAVE RESULT
          ret
esc0p:    pop       di                  ; CLEAN UP STACK
esc0:     mov       word ptr [opcode],offset dg:escmn
          mov       al,dl
          mov       di,offset dg:opbuf
          jmp short esc1
esc:      call      get64
esc1:     call      savhex
          cmp       byte ptr [mode],3
          jz        shrtesc
          mov       byte ptr  [aword],1
          jmp       memop2
shrtesc:  mov       al,","
          stosb
          mov       al,[regmem]
          and       al,7
          jmp       savreg
invarw:   call      putax
          jmp short invar
invarb:   call      putal
invar:    mov       al,','
          stosb
          jmp       putdx
infixw:   call      putax
          jmp short infix
infixb:   call      putal
infix:    mov     al,','
          stosb
          jmp       sav8
          stosw
          ret
outvarb:  mov       bx,'LA'
          jmp       short outvar
outvarw:  mov       bx,'XA'
outvar:   call      putdx
outfv:    mov     al,','
          stosb
          mov       ax,bx
          stosw
          ret
outfixb:  mov       bx,'LA'
          jmp short outfix
outfixw:  mov       bx,'XA'
outfix:   call      sav8
          jmp       outfv
putal:    mov       ax,'A'+4C00h        ; "AL"
          jmp short putx
putax:    mov       ax,'A'+5800h        ; "AX"
          jmp short putx
putdx:    mov       ax,'D'+5800h        ; "DX"
putx:     stosw
          ret
shft:     mov       bx,offset dg:shftab
          call      getmne
testreg:  cmp       byte ptr [mode],3
          jz        noflg
          mov       si,offset dg:size_tab
          mov       cl,3
          test      byte ptr [aword],-1
          jnz       test_1
          inc       cl
test_1:   call      movbyt
noflg:    jmp       addrmod
shiftv:   call      shft
          mov       al,','
          stosb
          mov       word ptr [di],'C'+4C00H ; "CL"
          ret
shift:    call      shft
          mov       ax,'1,'
          stosw
          ret
getmne:   call      getmode
          mov       dl,al
          cbw
          shl       ax,1
          add       bx,ax
          mov       ax,[bx]
          mov       [opcode],ax
          mov       al,dl
          ret
grp1:     mov       bx,offset dg:grp1tab
          call      getmne
          or        al,al
          jz        finimmj
          jmp       testreg
finimmj:  jmp       finimm
grp2:     mov       bx,offset dg:grp2tab
          call      getmne
          cmp       al,2
          jb        testreg
          cmp       al,6
          jae       indirect
          test      al,1
          jz        indirect
          mov       ax,'AF'             ; "FAR"
          stosw
          mov       ax,' R'
          stosw
indirect: jmp       addrmod
code      ends
          end
