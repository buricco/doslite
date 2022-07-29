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

; Code for the ASSEMble command in the debugger

          include   debequ.inc
          include   dossym.inc

code      segment   public byte 'code'
code      ends

const     segment   public byte
          extrn     dbmn:byte,cssave:word,reg8:byte,reg16:byte,siz8:byte
          extrn     synerr:byte,optab:byte,maxop:abs
const     ends

data      segment   public byte
          extrn     hinum:word,lownum:word,assemcnt:byte
          extrn     assem1:byte,assem2:byte,assem3:byte,assem4:byte,assem5:byte
          extrn     assem6:byte,opbuf:byte,opcode:word,regmem:byte,index:word
          extrn     asmadd:byte,asmsp:word,movflg:byte,segflg:byte,tstflg:byte
          extrn     numflg:byte,dirflg:byte,bytebuf:byte,f8087:byte,diflg:byte
          extrn     siflg:byte,bxflg:byte,bpflg:byte,negflg:byte,memflg:byte
          extrn     regflg:byte,aword:byte,midfld:byte,mode:byte
data      ends

dg        group     code,const,data

code      segment public byte 'code'
assume    cs:dg,ds:dg,es:dg,ss:dg
          public    assem
          public    db_oper,dw_oper,assemloop,group2,aa_oper,dcinc_oper
          public    group1,esc_oper,fgroupp,fgroupx,fde_oper,fgroupz
          public    fd9_oper,fgroup,fdb_oper,fgroupb,fgroup3,fgroup3w
          public    fgroupds,int_oper,in_oper,disp8_oper,jmp_oper,no_oper
          public    out_oper,l_oper,mov_oper,pop_oper,push_oper,rotop
          public    tst_oper,ex_oper,get_data16,call_oper
          extrn     inbuf:near,scanb:near,scanp:near,gethx:near,get_address:near
          extrn     default:near,outdi:near,blank:near,printmes:near,tab:near

;
;         Line by line assembler
;

assem:    mov       bp,[cssave]         ; Default code segment
          mov       di,offset dg:asmadd ; Default address
          call      default
          mov       word ptr [asmadd],dx
          mov       word ptr [asmadd+2],ax
          mov       [asmsp],sp          ; Save sp in case of error
assemloop:
          mov       sp,[asmsp]          ; Restore sp in case of error
          les       di,dword ptr asmadd ; GET PC
          call      outdi               ; OUTPUT ADDRESS
          call      blank               ; SKIP A SPACE
          push      cs
          pop       es
          call      inbuf               ; GET A BUFFER
          call      scanb
          jnz       oplook
          ret                           ; IF EMPTY JUST RETURN
;
;  At this point ds:si points to the opcode mnemonic...
;
oplook:   xor       cx,cx               ; OPCODE COUNT = 0
          mov       di,offset dg:dbmn
opscan:   xor       bx,bx
oploop:   mov       al,[di+bx]
          and       al,7fh
          cmp       al,[si+bx]
          jz        opmatch
          inc       cx                  ; INCREMENT OPCODE COUNT
          cmp       cx,maxop            ; CHECK FOR END OF LIST
          jb        op1
          jmp       asmerr
op1:      inc       di                  ; SCAN FOR NEXT OPCODE...
          test      byte ptr [di-1],80h
          jz        op1
          jmp       opscan

opmatch:  inc       bx                  ; COMPARE NEXT CHAR
          test      byte ptr [di+bx-1],80h
          jz        oploop              ; IF NOT DONE KEEP COMPARING
          xchg      bx,cx
          mov       ax,bx
          shl       ax,1
          add       ax,bx
          add       ax,offset dg:optab
          mov       bx,ax
;
; CX = COUNT OF CHARS IN OPCODE
; BX = POINTER INTO OPCODE TABLE
;
          xor       ax,ax
          mov       byte ptr [aword],al
          mov       word ptr [movflg],ax
          mov       byte ptr [segflg],al
          mov       ah,00001010b        ; SET UP FOR AA_OPER
          mov       al,byte ptr [bx]
          mov       word ptr [assem1],ax
          mov       byte ptr [assemcnt],1

          add       si,cx               ; SI POINTS TO OPERAND
          jmp       word ptr [bx+1]
;
; 8087 INSTRUCTIONS WITH NO OPERANDS
;
fde_oper: mov       ah,0deh
          jmp       short fdx_oper
fdb_oper: mov       ah,0dbh
          jmp       short fdx_oper
fd9_oper: mov       ah,0d9h
fdx_oper: xchg      al,ah
          mov       word ptr [assem1],ax
;
;  aad and aam instrucions
;
aa_oper:  inc       byte ptr [assemcnt]
;
;  instructions with no operands
;
no_oper:  call      stuff_bytes
          call      scanp
          push      cs
          pop       es
          jnz       oplook
          jmp       assemloop
;
;  push instruction
;
push_oper:
          mov       ah,11111111b
          jmp       short pop1
;
;  pop instruction
;
pop_oper: mov       ah,10001111b
pop1:     mov       [assem1],ah
          mov       [midfld],al
          inc       byte ptr [movflg]   ; ALLOW SEGMENT REGISTERS
          mov       byte ptr [aword],2  ; MUST BE 16 BITS
          call      getregmem
          call      buildit
          mov       al,[di+2]
          cmp       al,11000000b
          jb        datret
          mov       byte ptr [di],1
          cmp       byte ptr [movflg],2
          jnz       pop2
          and       al,00011000b
          or        al,00000110b
          cmp       byte ptr [midfld],0
          jnz       pop3
          or        al,00000001b
          jmp       short pop3
pop2:     and       al,111b
          or        al,01010000b
          cmp       byte ptr [midfld],0
          jnz       pop3
          or        al,01011000b
pop3:     mov       byte ptr [di+1],al
          jmp       assem_exit
;
; ret and retf instructions
;
get_data16:
          call      scanb
          mov       cx,4
          call      gethx
          jc        datret
          dec       byte ptr [assem1]   ; CHANGE OPCODE
          add       byte ptr [assemcnt],2
          mov       word ptr [assem2],dx
datret:   jmp       assem_exit
;
;  int instruction
;
int_oper: call      scanb
          mov       cx,2
          call      gethx
          jc        errv1
          mov       al,dl
          cmp       al,3
          jz        datret
          inc       byte ptr [assem1]
          jmp       dispx
;
;  in instruction
;
in_oper:  call      scanb
          lodsw
          cmp       ax,'A'+4C00H        ; "AL"
          jz        in_1
          cmp       ax,'A'+5800H        ; "AX"
          jz        in_0
errv1:    jmp       asmerr
in_0:     inc       byte ptr [assem1]
in_1:     call      scanp
          cmp       word ptr [si],'D'+5800H
          jz        datret
          mov       cx,2
          call      gethx
          jc        errv1
          and       byte ptr [assem1],11110111b
          mov       al,dl
          jmp       dispx
;
;  out instruction
;
out_oper:
          call      scanb
          cmp       word ptr [si],'D'+5800H
          jnz       out_0
          inc       si
          inc       si
          jmp       short out_1
out_0:    and       byte ptr [assem1],11110111b
          mov       cx,2
          call      gethx
          jc        errv1
          inc       byte ptr [assemcnt]
          mov       byte ptr [assem2],dl
out_1:    call      scanp
          lodsw
          cmp       ax,'A'+4C00H        ; "AL"
          jz        datret
          cmp       ax,'A'+5800H        ; "AX"
          jnz       errv1
          inc       byte ptr [assem1]
          jmp       datret

;
;  jump instruction
;
jmp_oper: inc       byte ptr [tstflg]
;
;  call instruction
;
call_oper:
          mov       byte ptr [assem1],11111111b
          mov       byte ptr [midfld],al
          call      getregmem
          call      build3
          cmp       byte ptr [memflg],0
          jnz       callj1
          cmp       byte ptr [regmem],-1
          jz        callj2
;
;  INDIRECT JUMPS OR CALLS
;
callj1:   cmp       byte ptr [aword],1
errz4:    jz        errv1
          cmp       byte ptr [aword],4
          jnz       asmex4
          or        byte ptr [di+2],1000b
          jmp short asmex4
;
;   DIRECT JUMPS OR CALLS
;
callj2:   mov       ax,[lownum]
          mov       dx,[hinum]
          mov       bl,[aword]
          cmp       byte ptr [numflg],0
          jz        errz4

;  BL = NUMBER OF BYTES IN JUMP
;  DX =   OFFSET
;  AX =   SEGMENT

callj3:   mov       byte ptr [di],5
          mov       [di+2],ax
          mov       [di+4],dx
          mov       al,10011010b        ; SET UP INTER SEGMENT CALL
          cmp       byte ptr [tstflg],0
          jz        callj5
          mov       al,11101010b        ; FIX UP FOR JUMP
callj5:   mov       byte ptr [di+1],al
          cmp       bl,4                ; FAR SPECIFIED?
          jz        asmex4
          or        bl,bl
          jnz       callj6
          cmp       dx,word ptr [asmadd+2]
          jnz       asmex4
callj6:   mov       byte ptr [di],3
          mov       al,11101000b        ; SET UP FOR INTRASEGMENT
          or        al,[tstflg]
          mov       byte ptr [di+1],al
          mov       ax,[lownum]
          sub       ax,word ptr [asmadd]
          sub       ax,3
          mov       [di+2],ax
          cmp       byte ptr [tstflg],0
          jz        asmex4
          cmp       bl,2
          jz        asmex4
          inc       ax
          mov       cx,ax
          cbw
          cmp       ax,cx
          jnz       asmex3
          mov       byte ptr [di+1],11101011b
          mov       [di+2],ax
          dec       byte ptr [di]
asmex4:   jmp       assem_exit
;
;  conditional jumps and loop instructions
;
disp8_oper:
          mov       bp,word ptr [asmadd+2]
          call      get_address
          sub       dx,word ptr [asmadd]
          dec       dx
          dec       dx
          call      chksiz
          cmp       cl,1
          jnz       errv2
dispx:    inc       [assemcnt]
          mov       byte ptr [assem2],al
asmex3:   jmp       assem_exit
;
;  lds, les, and lea instructions
;
l_oper:
          call      scanb
          lodsw
          mov       cx,8
          mov       di,offset dg:reg16
          call      chkreg
          jz        errv2               ; CX = 0 MEANS NO REGISTER
          shl       al,1
          shl       al,1
          shl       al,1
          mov       byte ptr [midfld],al
          call      scanp
          call      getregmem
          cmp       byte ptr [aword],0
          jnz       errv2
          call      build2
          jmp       short asexv
;
;  dec and inc instructions
;
dcinc_oper:
          mov       byte ptr [assem1],11111110b
          mov       byte ptr [midfld],al
          call      getregmem
          call      buildit
          test      byte ptr [di+1],1
          jz        asexv
          mov       al,[di+2]
          cmp       al,11000000b
          jb        asexv
          and       al,1111b
          or        al,01000000b
          mov       [di+1],al
          dec       byte ptr [di]
asexv:    jmp       assem_exit

errv2:    jmp       asmerr
;
; esc instruction
;
esc_oper: inc       byte ptr [aword]
          call      scanb
          mov       cx,2
          call      gethx
          cmp       dx,64
          jae       errv2
          call      scanp
          mov       ax,dx
          mov       cl,3
          shr       dx,cl
          or        [assem1],dl
          and       al,111b
          shl       al,cl
          jmp       groupe
;
; 8087 arithmetic instuctions
;

;
;  OPERANDS THAT ALLOW THE REVERSE BIT
;
fgroupds: call      setmid
          call      getregmem2
          call      build3
          cmp       byte ptr [mode],11000000b
          jnz       fgroup1
          mov       al,[dirflg]
          or        al,al
          jz        fexit
          or        [di+1],al           ; IF D=1...
          xor       byte ptr [di+2],00001000b ; ...REVERSE THE SENSE OF R
          jmp       short fexit

;
;  Here when instruction could have memory or register operand
;
fgroupx:  call      setmid              ; THIS ENTRY POINT FOR 1 MEM OPER
          mov       byte ptr [dirflg],0
          jmp       short fgrp2
fgroup:   call      setmid
fgrp2:    call      getregmem2
          call      build3
          cmp       byte ptr [mode],11000000b
          jnz       fgroup1
          mov       al,[dirflg]
          or        [di+1],al
          jmp       short fexit
fgroup1:  call      setmf
fexit:    jmp       assem_exit
;
; These 8087 instructions require a memory operand
;
fgroupb:  mov       ah,5                ; MUST BE TBYTE
          jmp       short fgroup3e
fgroup3w: mov       ah,2                ; MUST BE WORD
          jmp       short fgroup3e
fgroup3:  mov       ah,-1               ; SIZE CANNOT BE SPECIFIED
fgroup3e: mov       [aword],ah
          call      setmid
          call      getregmem
          cmp       byte ptr [mode],11000000b
          jz        fgrperr
fgrp:     call      build3
          jmp       fexit
;
; These 8087 instructions require a register operand
;
fgroupp:  mov       byte ptr [aword],-1 ; 8087 POP OPERANDS
          call      setmid
          call      getregmem2
          cmp       byte ptr [dirflg],0
          jnz       fgrp
fgrperr:  jmp     asmerr

fgroupz:  call      setmid              ; ENTRY POINT WHERE ARG MUST BE MEM
          mov       byte ptr [dirflg],0
          call      getregmem
          cmp       byte ptr [mode],11000000b
          jz        fgrperr
          call      build3
          call      setmf
          jmp       fexit
;
; not, neg, mul, imul, div, and idiv instructions
;
group1:   mov       [assem1],11110110b
groupe:   mov       byte ptr [midfld],al
          call      getregmem
          call      buildit
          jmp       fexit
;
;  shift and rotate instructions
;
rotop:    mov       [assem1],11010000b
          mov       byte ptr [midfld],al
          call      getregmem
          call      buildit
          call      scanp
          cmp       byte ptr [si],'1'
          jz        asmexv1
          cmp       word ptr [si],'LC'  ; CL
          jz        rotop1
roterr:   jmp       asmerr
rotop1:   or        byte ptr [assem1],10b
asmexv1:  jmp       assem_exit
;
;  xchg instruction
;
ex_oper:  inc       byte ptr [tstflg]
;
;   test instruction
;
tst_oper: inc       byte ptr [tstflg]
          jmp short movop
;
;    mov instruction
;
mov_oper: inc       byte ptr [movflg]
movop:    xor       ax,ax
          jmp short groupm
;
;   add, adc, sub, sbb, cmp, and, or, xor instructions
;
group2:   mov       byte ptr [assem1],10000000b
groupm:   mov       byte ptr [midfld],al
          push      ax
          call      getregmem
          call      build2
          call      scanp               ; POINT TO NEXT OPERAND
          mov       al,byte ptr [assemcnt]
          push      ax
          call      getregmem
          pop       ax
          mov       byte ptr [di],al
          pop       ax
          mov       bl,byte ptr [aword]
          or        bl,bl
          jz        errv5
          dec       bl
          and       bl,1
          or        byte ptr [di+1],bl
          cmp       byte ptr [memflg],0
          jnz       g21v
          cmp       byte ptr [numflg],0 ; TEST FOR IMMEDIATE DATA
          jz        g21v
          cmp       byte ptr [segflg],0
          jnz       errv5
          cmp       byte ptr [tstflg],2 ; XCHG?
          jnz       immed1
errv5:    jmp       asmerr
g21v:     jmp       grp21
;
;  SECOND OPERAND WAS IMMEDIATE
;
immed1:   mov       al,byte ptr [di+2]
          cmp       byte ptr [movflg],0
          jz        notmov1
          and       al,11000000b
          cmp       al,11000000b
          jnz       grp23               ; not to a register
                                        ; MOVE IMMEDIATE TO REGISTER
          mov       al,byte ptr [di+1]
          and       al,1                ; SET SIZE
          pushf
          shl       al,1
          shl       al,1
          shl       al,1
          or        al,byte ptr [di+2]  ; SET REGISTER
          and       al,00001111b
          or        al,10110000b
          mov       byte ptr [di+1],al
          mov       ax,word ptr [lownum]
          mov       word ptr [di+2],ax
          popf
          jz        exvec
          inc       byte ptr [di]
exvec:    jmp       grpex
notmov1:  and       al,11000111b
          cmp       al,11000000b
          jz        immacc              ; IMMEDIATE TO ACC
          cmp       byte ptr [tstflg],0
          jnz       grp23
          cmp       byte ptr [midfld],1*8   ; OR?
          jz        grp23
          cmp       byte ptr [midfld],4*8   ; AND?
          jz        grp23
          cmp       byte ptr [midfld],6*8   ; XOR?
          jz        grp23
          test      byte ptr [di+1],1       ; TEST IF BYTE OPCODE
          jz        grp23
          mov       ax,[lownum]
          mov       bx,ax
          cbw
          cmp       ax,bx
          jnz       grp23                   ; SMALL ENOUGH?
          mov       bl,[di]
          dec       byte ptr [di]
          or        byte ptr [di+1],10b
          jmp       short grp23x

immacc:   mov       al,byte ptr [di+1]
          and       al,1
          cmp       byte ptr [tstflg],0
          jz        nottst
          or        al,10101000b
          jmp       short test1
nottst:   or        al,byte ptr [midfld]
          or        al,100b
test1:    mov       byte ptr [di+1],al
          dec       byte ptr [di]

grp23:    mov       bl,byte ptr [di]
grp23x:   xor       bh,bh
          add       bx,di
          inc       bx
          mov       ax,word ptr [lownum]
          mov       word ptr [bx],ax
          inc       byte ptr [di]
          test      byte ptr [di+1],1
          jz        grpex1
          inc       byte ptr [di]
grpex1:   jmp       grpex
;
;       SECOND OPERAND WAS MEMORY OR REGISTER
;
grp21:
          cmp       byte ptr [segflg],0
          jz        grp28                   ; FIRST OPERAND WAS A SEGMENT REG
          mov       al,byte ptr [regmem]
          test      al,10000b
          jz        notseg1
errv3:    jmp       asmerr
notseg1:  and       al,111b
          or        byte ptr [di+2],al
          and       byte ptr [di+1],11111110b
          cmp       byte ptr [memflg],0
          jnz       g22v
          jmp       grpex

grp28:    and       byte ptr [di+2],11000111b
          mov       al,byte ptr [di+1]      ; GET FIRST OPCODE
          and       al,1b
          cmp       byte ptr [movflg],0
          jz        notmov2
          or        al,10001000b
          jmp short mov1
notmov2:  cmp       byte ptr [tstflg],0
          jz        nottst2
          or        al,10000100b
          cmp       byte ptr [tstflg],2
          jnz       nottst2
          or        al,10b
nottst2:  or        al,byte ptr [midfld]    ; MIDFLD IS ZERO FOR TST
mov1:     mov       byte ptr [di+1],al
          cmp       byte ptr [memflg],0
g22v:     jnz       grp22
;
;       SECOND OPERAND WAS A REGISTER
;
          mov       al,byte ptr [regmem]
          test      al,10000b               ; SEGMENT REGISTER?
          jz        notseg
          cmp       byte ptr [movflg],0
          jz        errv3
          mov       byte ptr [di+1],10001100b

notseg:   and       al,111b
          shl       al,1
          shl       al,1
          shl       al,1
          or        byte ptr [di+2],al
;
; SPECIAL FORM OF THE EXCHANGE COMMAND
;
          cmp       byte ptr [tstflg],2
          jnz       grpex
          test      byte ptr [di+1],1
          jz        grpex
          push      ax
          mov       al,byte ptr [di+2]
          and       al,11000000b
          cmp       al,11000000b            ; MUST BE REGISTER TO REGISTER
          pop       ax
          jb        grpex
          or        al,al
          jz        specx
          mov       al,[di+2]
          and       al,00000111b
          jnz       grpex
          mov       cl,3
          shr       byte ptr [di+2],cl
specx:    mov       al,[di+2]
          and       al,00000111b
          or        al,10010000b
          mov       byte ptr [di+1],al
          dec       byte ptr [di]
          jmp short grpex
;
;  SECOND OPERAND WAS A MEMORY REFERENCE
;
grp22:    cmp       byte ptr [tstflg],0
          jnz       tst2
          or        byte ptr [di+1],10b
tst2:     mov       al,byte ptr [di+2]
          cmp       al,11000000b            ; MUST BE A REGISTER
          jb        asmerr
          cmp       byte ptr [segflg],0
          jz        grp223
          and       al,00011000b
          jmp       short grp222
grp223:   and       al,111b
          shl       al,1
          shl       al,1
          shl       al,1
grp222:   or        al,byte ptr [mode]
          or        al,byte ptr [regmem]
          mov       byte ptr [di+2],al
          mov       ax,word ptr [lownum]
          mov       word ptr [di+3],ax
grpsiz:   mov       byte ptr [di],2
          mov       al,byte ptr [di+2]
          and       al,11000111b
          cmp       al,00000110b
          jz        grp24
          and       al,11000000b
          cmp       al,01000000b
          jz        grp25
          cmp       al,10000000b
          jnz       grpex
grp24:    inc       byte ptr [di]
grp25:    inc       byte ptr [di]

grpex:    cmp       byte ptr [movflg],0
          jz        assem_exit
;
;       TEST FOR SPECIAL FORM OF MOV AX,[MEM] OR MOV [MEM],AX
;
          mov       al,[di+1]               ; GET OPCODE
          and       al,11111100b
          cmp       al,10001000b
          jnz       assem_exit
          cmp       byte ptr [di+2],00000110b   ; MEM TO AX OR AX TO MEM
          jnz       assem_exit
          mov       al,byte ptr [di+1]
          and       al,11b
          xor       al,10b
          or        al,10100000b
          mov       byte ptr [di+1],al
          dec       byte ptr [di]
          mov       ax,[di+3]
          mov       word ptr [di+2],ax

assem_exit:
          call      stuff_bytes
          jmp       assemloop

; Assem error. SI points to character in the input buffer
; which caused error. By subtracting from start of buffer,
; we will know how far to tab over to appear directly below
; it on the terminal. Then print "^ Error".

asmerr:
          sub       si,offset dg:(bytebuf-10)   ; How many char processed so far?
          mov       cx,si                   ; Parameter for TAB in CX
          call      tab                     ; Directly below bad char
          mov       si,offset dg:synerr     ; Error message
          call      printmes
          jmp       assemloop
;
;  assemble the different parts into an instruction
;
buildit:
          mov       al,byte ptr [aword]
          or        al,al
          jnz       build1
blderr:   jmp       asmerr

build1:   dec       al
          or        byte ptr [di+1],al      ; SET THE SIZE

build2:   cmp       byte ptr [numflg],0     ; TEST FOR IMMEDIATE DATA
          jz        build3
          cmp       byte ptr [memflg],0
          jz        blderr

build3:   mov       al,byte ptr [regmem]
          cmp       al,-1
          jz        bld1
          test      al,10000b               ; TEST IF SEGMENT REGISTER
          jz        bld1
          cmp       byte ptr [movflg],0
          jz        blderr
          mov       word ptr [di+1],10001110b
          inc       byte ptr [movflg]
          inc       byte ptr [segflg]
          and       al,00000011b
          shl       al,1
          shl       al,1
          shl       al,1
          or        al,byte ptr 11000000b
          mov       byte ptr [di+2],al
          ret

bld1:     and       al,00000111b
bld4:     or        al,byte ptr [mode]
          or        al,byte ptr [midfld]
          mov       byte ptr [di+2],al
          mov       ax,word ptr [lownum]
          mov       word ptr [di+3],ax
          ret

getregmem:
          mov       byte ptr [f8087],0
getregmem2:
          call      scanp
          xor       ax,ax
          mov       word ptr [lownum],ax    ; OFFSET
          mov       word ptr [diflg],ax     ; DIFLG+SIFLG
          mov       word ptr [bxflg],ax     ; BXFLG+BPFLG
          mov       word ptr [negflg],ax    ; NEGFLG+NUMFLG
          mov       word ptr [memflg],ax    ; MEMFLG+REGFLG
          dec       al
          cmp       byte ptr [f8087],0
          jz        putreg
          mov       al,1                    ; DEFAULT 8087 REG IS 1
putreg:   mov       byte ptr [regmem],al

getloop:mov     byte ptr [negflg],0
getloop1:
          mov       ax,word ptr [si]
          cmp       al,','
          jz        gomode
          cmp       al,13
          jz        gomode
          cmp       al,';'
          jz        gomode
          cmp       al,9
          jz        gettb
          cmp       al,' '
          jnz       goget
gettb:    inc       si
          jmp       getloop1
goget:    jmp       getinfo
;
;  DETERMINE THE MODE BITS
;
gomode:   mov       di,offset dg:assemcnt
          mov       byte ptr [mode],11000000b
          mov       byte ptr [assemcnt],2
          cmp       byte ptr [memflg],0
          jnz       gomode1
          mov       al,[numflg]
          or        al,[regflg]
          jnz       moret
          or        al,[f8087]
          jz        erret
          mov       al,[di+1]
          or        al,[dirflg]
          cmp       al,0dch                 ; ARITHMETIC?
          jnz       moret
          mov       byte ptr [di+1],0deh    ; ADD POP TO NULL ARG 8087
moret:    ret
erret:    jmp       asmerr

gomode1:mov     byte ptr [mode],0
          cmp       byte ptr [numflg],0
          jz        goregmem

          mov       byte ptr [di],4
          mov       ax,word ptr [diflg]
          or        ax,word ptr [bxflg]
          jnz       gomode2
          mov       byte ptr [regmem],00000110b
          ret

gomode2:mov     byte ptr [mode],10000000b
          call      chksiz1
          cmp       cl,2
          jz        goregmem
          dec       byte ptr [di]
          mov       byte ptr [mode],01000000b
;
;  DETERMINE THE REG-MEM BITS
;
goregmem:
          mov       bx,word ptr [bxflg]
          mov       cx,word ptr [diflg]
          xor       dx,dx
goreg0:
          mov       al,bl                   ; BX
          add       al,ch                   ; SI
          cmp       al,2
          jz        gogo
          inc       dl
          mov       al,bl
          add       al,cl
          cmp       al,2
          jz        gogo
          inc       dl
          mov       al,bh
          add       al,ch
          cmp       al,2
          jz        gogo
          inc       dl
          mov       al,bh
          add       al,cl
          cmp       al,2
          jz        gogo
          inc       dl
          or        ch,ch
          jnz       gogo
          inc       dl
          or        cl,cl
          jnz       gogo
          inc       dl                      ; BP+DISP
          or        bh,bh
          jz        goreg1
          cmp       byte ptr [mode],0
          jnz       gogo
          mov       byte ptr [mode],01000000b
          inc       byte ptr [di]
          dec       dl
goreg1:   inc       dl                      ; BX+DISP
gogo:     mov       byte ptr [regmem],dl
          ret

getinfo:  cmp       ax,'EN'                 ; NEAR
          jnz       getreg3
getreg0:  mov       dl,2
getrg01:  call      setsiz1
getreg1:  call      scans
          mov       ax,word ptr [si]
          cmp       ax,'TP'                 ; PTR
          jz        getreg1
          jmp       getloop

getreg3:  mov       cx,5
          mov       di,offset dg:siz8
          call      chkreg                  ; LOOK FOR BYTE, WORD, DWORD, ETC.
          jz        getreg41
          inc       al
          mov       dl,al
          jmp       getrg01

getreg41:
          mov       ax,[si]
          cmp       byte ptr [f8087],0
          jz        getreg5
          cmp       ax,'TS'                 ; 8087 STACK OPERAND
          jnz       getreg5
          cmp       byte ptr [si+2],','
          jnz       getreg5
          mov       byte ptr [dirflg],0
          add       si,3
          jmp       getloop

getreg5:  cmp       ax,'HS'                 ; SHORT
          jz        getreg1

          cmp       ax,'AF'                 ; FAR
          jnz       getrg51
          cmp       byte ptr [si+2],'R'
          jnz       getrg51
          add       si,3
          mov       dl,4
          jmp       getrg01

getrg51:  cmp       al,'['
          jnz       getreg7
getreg6:  inc       byte ptr [memflg]
          inc       si
          jmp       getloop

getreg7:  cmp       al,']'
          jz        getreg6
          cmp       al,'.'
          jz        getreg6
          cmp       al,'+'
          jz        getreg6
          cmp       al,'-'
          jnz       getreg8
          inc       byte ptr [negflg]
          inc       si
          jmp       getloop1

getreg8:                                ; LOOK FOR A REGISTER
          cmp       byte ptr [f8087],0
          jz        getregreg
          cmp       ax,"TS"
          jnz       getregreg
          cmp       byte ptr [si+2],'('
          jnz       getregreg
          cmp       byte ptr [si+4],')'
          jnz       asmpop
          mov       al,[si+3]
          sub       al,'0'
          jb        asmpop
          cmp       al,7
          ja        asmpop
          mov       [regmem],al
          inc       byte ptr [regflg]
          add       si,5
          cmp       word ptr [si],'S,'
          jnz       zloop
          cmp       byte ptr [si+2],'T'
          jnz       zloop
          add       si,3
zloop:    jmp       getloop

getregreg:
          mov       cx,20
          mov       di,offset dg:reg8
          call      chkreg
          jz        getreg12                ; CX = 0 MEANS NO REGISTER
          mov       byte ptr [regmem],al
          inc       byte ptr [regflg]       ; TELL EVERYONE WE FOUND A REG
          cmp       byte ptr [memflg],0
          jnz       nosize
          call      setsiz
incsi2:   add       si,2
          jmp       getloop

nosize:   cmp       al,11                   ; BX REGISTER?
          jnz       getreg9
          cmp       word ptr [bxflg],0
          jz        getok
asmpop:   jmp       asmerr

getok:    inc       byte ptr [bxflg]
          jmp       incsi2
getreg9:
          cmp       al,13                   ; BP REGISTER?
          jnz       getreg10
          cmp       word ptr [bxflg],0
          jnz       asmpop
          inc       byte ptr [bpflg]
          jmp       incsi2
getreg10:
          cmp       al,14                   ; SI REGISTER?
          jnz       getreg11
          cmp       word ptr [diflg],0
          jnz       asmpop
          inc       byte ptr [siflg]
          jmp       incsi2
getreg11:
          cmp       al,15                   ; DI REGISTER?
          jnz       asmpop                  ; *** error
          cmp       word ptr [diflg],0
          jnz       asmpop
          inc       byte ptr [diflg]
          jmp       incsi2

getreg12:                               ; BETTER BE A NUMBER!
          mov       bp,word ptr [asmadd+2]
          cmp       byte ptr [memflg],0
          jz        gtrg121
gtrg119:mov     cx,4
gtrg120:call    gethx
          jmp       short gtrg122
gtrg121:mov     cx,2
          cmp       byte ptr [aword],1
          jz        gtrg120
          cmp       byte ptr [aword],cl
          jz        gtrg119
          call      get_address
gtrg122:jc      asmpop
          mov       [hinum],ax
          cmp       byte ptr [negflg],0
          jz        getreg13
          neg       dx
getreg13:
          add       word ptr [lownum],dx
          inc       byte ptr [numflg]
getloopv:
          jmp       getloop

chkreg:   push      cx
          inc       cx
          repnz     scasw
          pop       ax
          sub       ax,cx
          or        cx,cx
          ret

stuff_bytes:
          push      si
          les       di,dword ptr asmadd
          mov       si,offset dg:assemcnt
          xor       ax,ax
          lodsb
          mov       cx,ax
          jcxz      stuffret
          rep       movsb
          mov       word ptr [asmadd],di
stuffret:
          pop       si
          ret

setsiz:
          mov       dl,1
          test      al,11000b               ; 16 BIT OR SEGMENT REGISTER?
          jz        setsiz1
          inc       dl
setsiz1:
          cmp       byte ptr [aword],0
          jz        setsiz2
          cmp       byte ptr [aword],dl
          jz        setsiz2
seterr:   pop       dx
          jmp       asmpop
setsiz2:  mov       byte ptr [aword],dl
          ret
;
;  DETERMINE IF NUMBER IN AX:DX IS 8 BITS, 16 BITS, OR 32 BITS
;
chksiz:   mov       cl,4
          cmp       ax,bp
          jnz       retchk
chksiz1:  mov       cl,2
          mov       ax,dx
          cbw
          cmp       ax,dx
          jnz       retchk
          dec       cl
retchk:   ret
;
;  get first character after first space
;
scans:    cmp       byte ptr [si],13
          jz        retchk
          cmp       byte ptr [si],"["
          jz        retchk
          lodsb
          cmp       al," "
          jz        scanbv
          cmp       al,9
          jnz       scans
scanbv:   jmp       scanb
;
; Set up for 8087 op-codes
;
setmid:   mov       byte ptr [assem1],0d8h
          mov       ah,al
          and       al,111b                 ; SET MIDDLE BITS OF SECOND BYTE
          shl       al,1
          shl       al,1
          shl       al,1
          mov       [midfld],al
          mov       al,ah                   ; SET LOWER BITS OF FIRST BYTE
          shr       al,1
          shr       al,1
          shr       al,1
          or        [assem1],al
          mov       byte ptr [f8087],1      ; INDICATE 8087 OPERAND
          mov       byte ptr [dirflg],100b
          ret
;
; Set MF bits for 8087 op-codes
;
setmf:    mov       al,[aword]
          test      byte ptr [di+1],10b
          jnz       setmfi
          and       byte ptr [di+1],11111001b
          cmp       al,3                    ; DWORD?
          jz        setmfret
          cmp       al,4                    ; QWORD?
          jz        setmfret2
          test      byte ptr [di+1],1
          jz        setmferr
          cmp       al,5                    ; TBYTE?
          jz        setmfret3
          jmp short setmferr
setmfi:   cmp       al,3                    ; DWORD?
          jz        setmfret
          cmp       al,2                    ; WORD?
          jz        setmfret2
          test      byte ptr [di+1],1
          jz        setmferr
          cmp       al,4                    ; QWORD?
          jnz       setmferr
          or        byte ptr [di+1],111b
setmfret3:
          or        byte ptr [di+1],011b
          or        byte ptr [di+2],101000b
          jmp       short setmfret
setmfret2:
          or        byte ptr [di+1],100b
setmfret:
          ret

setmferr:
          jmp       asmpop


dw_oper:
          mov       bp,1
          jmp       short dben

db_oper:  xor       bp,bp
dben:     mov       di,offset dg:assemcnt
          dec       byte ptr [di]
          inc       di
db0:      xor       bl,bl
          call      scanp
          jnz       db1
dbex:     jmp       assem_exit
db1:      or        bl,bl
          jnz       db3
          mov       bh,byte ptr [si]
          cmp       bh,"'"
          jz        db2
          cmp       bh,'"'
          jnz       db4
db2:      inc       si
          inc       bl
db3:      lodsb
          cmp       al,13
          jz        dbex
          cmp       al,bh
          jz        db0
          stosb
          inc       byte ptr [assemcnt]
          jmp       db3
db4:      mov       cx,2
          cmp       bp,0
          jz        db41
          mov       cl,4
db41:     push      bx
          call      gethx
          pop       bx
          jnc       db5
          jmp       asmerr
db5:      mov       ax,dx
          cmp       bp,0
          jz        db6
          stosw
          inc       byte ptr [assemcnt]
          jmp       short db7
db6:      stosb
db7:      inc       byte ptr [assemcnt]
          jmp       db0

code      ends
          end
