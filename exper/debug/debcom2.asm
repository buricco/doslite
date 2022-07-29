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

; Routines to perform debugger commands except ASSEMble and UASSEMble

          include   debequ.inc
          include   dossym.inc

code      segment   public byte 'code'
code      ends

const     segment   public byte
          extrn     notfnd:byte,noroom:byte,drvlet:byte,nospace:byte,nambad:byte
          extrn     toobig:byte,errmes:byte
          extrn     exebad:byte,hexerr:byte,exewrt:byte,hexwrt:byte
          extrn     execemes:byte,wrtmes1:byte,wrtmes2:byte,accmes:byte

          extrn     flagtab:word,exec_block:byte,com_line:dword,com_fcb1:dword
          extrn     com_fcb2:dword,com_sssp:dword,com_csip:dword,retsave:word
          extrn     newexec:byte,headsave:word
          extrn     regtab:byte,totreg:byte,noregl:byte
          extrn     user_proc_pdb:word,stack:byte,rstack:word,axsave:word
          extrn     bxsave:word,dssave:word,essave:word,cssave:word,ipsave:word
          extrn     sssave:word,cxsave:word,spsave:word,_fsave:word
          extrn     sreg:byte,segtab:word,regdif:word,rdflg:byte
const     ends

data      segment   public byte
          extrn     defdump:byte,transadd:dword,index:word,buffer:byte
          extrn     asmadd:byte,disadd:byte,nseg:word,bptab:byte
          extrn     brkcnt:word,tcount:word,xnxcmd:byte,xnxopt:byte
          extrn     aword:byte,extptr:word,handle:word,parserr:byte
data      ends

dg        group     code,const,data

code      segment   public byte 'code'
assume    cs:dg,ds:dg,es:dg,ss:dg

          public    defio,skip_file,prepname,debug_found
          public    reg,compare,go,input,load
          public    _name,output,trace,ztrace,dwrite

          extrn     gethex:near,geteol:near
          extrn     crlf:near,blank:near,_out:near
          extrn     outsi:near,outdi:near,inbuf:near,scanb:near,scanp:near
          extrn     rprbuf:near,hex:near,out16:near,digit:near
          extrn     command:near,disasln:near,set_terminate_vector:near
          extrn     restart:near,dabort:near,terminate:near,drverr:near
          extrn     find_debug:near,nmiint:near,nmiintend:near
          extrn     hexchk:near,gethex1:near,print:near,dsrange:near
          extrn     address:near,hexin:near,perror:near

debcom2:
dispreg:
          mov       si,offset dg:regtab
          mov       bx,offset dg:axsave
          mov       byte ptr totreg,13
          mov       ch,0
          mov       cl,noregl
repdisp:
          sub       totreg,cl
          call      dispregline
          call      crlf
          mov       ch,0
          mov       cl,noregl
          cmp       cl,totreg
          jl        repdisp
          mov       cl,totreg
          call      dispregline
          call      blank
          call      dispflags
          call      crlf
          mov       ax,[ipsave]
          mov       word ptr [disadd],ax
          push      ax
          mov       ax,[cssave]
          mov       word ptr [disadd+2],ax
          push      ax
          mov       [nseg],-1
          call      disasln
          pop       word ptr disadd+2
          pop       word ptr disadd
          mov       ax,[nseg]
          cmp       al,-1
          jz        crlfj
          cmp       ah,-1
          jz        noover
          xchg      al,ah
noover:
          cbw
          mov       bx,ax
          shl       bx,1
          mov       ax,word ptr [bx+sreg]
          call      _out
          xchg      al,ah
          call      _out
          mov       al,":"
          call      _out
          mov       dx,[index]
          call      out16
          mov       al,"="
          call      _out
          mov       bx,[bx+segtab]
          push      ds
          mov       ds,[bx]
          mov       bx,dx
          mov       dx,[bx]
          pop       ds
          test      byte ptr [aword],-1
          jz        out8
          call      out16
crlfj:
          jmp       crlf
out8:
          mov       al,dl
          call      hex
          jmp       crlf

dispregj: jmp       dispreg

; Perform register dump if no parameters or set register if a
; register designation is a parameter.

reg:
          call      scanp
          jz        dispregj
          mov       dl,[si]
          inc       si
          mov       dh,[si]
          cmp       dh,13
          jz        flag
          inc       si
          call      geteol
          cmp       dh," "
          jz        flag
          mov       di,offset dg:regtab
          xchg      ax,dx
          push      cs
          pop       es
          mov       cx,regtablen
          repnz     scasw
          jnz       badreg
          or        cx,cx
          jnz       notpc
          dec       di
          dec       di
          mov       ax,cs:[di-2]
notpc:
          call      _out
          mov       al,ah
          call      _out
          call      blank
          push      ds
          pop       es
          lea       bx,[di+regdif-2]
          mov       dx,[bx]
          call      out16
          call      crlf
          mov       al,":"
          call      _out
          call      inbuf
          call      scanb
          jz        ret4
          mov       cx,4
          call      gethex1
          call      geteol
          mov       [bx],dx
ret4:     ret
badreg:
          mov       ax,5200h+'B'            ; BR ERROR
          jmp       err
flag:
          cmp       dl,"f"
          jnz       badreg
          call      dispflags
          mov       al,"-"
          call      _out
          call      inbuf
          call      scanb
          xor       bx,bx
          mov       dx,[_fsave]
getflg:
          lodsw
          cmp       al,13
          jz        savchg
          cmp       ah,13
          jz        flgerr
          mov       di,offset dg:flagtab
          mov       cx,32
          push      cs
          pop       es
          repne     scasw
          jnz       flgerr
          mov       ch,cl
          and       cl,0fh
          mov       ax,1
          rol       ax,cl
          test      ax,bx
          jnz       repflg
          or        bx,ax
          or        dx,ax
          test      ch,16
          jnz       nexflg
          xor       dx,ax
nexflg:
          call      scanp
          jmp       short getflg
dispregline:
          lods      cs:word ptr [si]
          call      _out
          mov       al,ah
          call      _out
          mov       al,'='
          call      _out
          mov       dx,[bx]
          inc       bx
          inc       bx
          call      out16
          call      blank
          call      blank
          loop      dispregline
          ret
repflg:
          mov       ax,4600h+'D'            ; DF ERROR
ferr:
          call      savchg
err:
          call      _out
          mov       al,ah
          call      _out
          mov       si,offset dg:errmes
          jmp       print
savchg:
          mov       [_fsave],dx
          ret
flgerr:
          mov       ax,4600h+'B'            ; BF ERROR
          jmp short ferr
dispflags:
          mov       si,offset dg:flagtab
          mov       cx,16
          mov       dx,[_fsave]
dflags:
          lods      cs:word ptr [si]
          shl       dx,1
          jc        flagset
          mov       ax,cs:[si+30]
flagset:
          or        ax,ax
          jz        nextflg
          call      _out
          mov       al,ah
          call      _out
          call      blank
nextflg:
          loop      dflags
          ret

; Input from the specified port and display result

input:
          mov       cx,4                    ; Port may have 4 digits
          call      gethex                  ; Get port number in DX
          call      geteol
          in        al,dx                   ; Variable port input
          call      hex                     ; And display
          jmp       crlf

; Output a value to specified port.

output:
          mov       cx,4                    ; Port may have 4 digits
          call      gethex                  ; Get port number
          push      dx                      ; Save while we get data
          mov       cx,2                    ; Byte output only
          call      gethex                  ; Get data to output
          call      geteol
          xchg      ax,dx                   ; Output data in AL
          pop       dx                      ; Port in DX
          out       dx,al                   ; Variable port output
ret5:     ret
compare:
          call      dsrange
          push      cx
          push      ax
          push      dx
          call      address                 ; Same segment
          call      geteol
          pop       si
          mov       di,dx
          mov       es,ax
          pop       ds
          pop       cx                      ; Length
          dec       cx
          call      comp                    ; Do one less than total
          inc       cx                      ; CX=1 (do last one)
comp:
          repe      cmpsb
          jz        ret5
          dec       si                      ; Compare error. Print address, value; value, address.
          call      outsi
          call      blank
          call      blank
          lodsb
          call      hex
          call      blank
          call      blank
          dec       di
          mov       al,es:[di]
          call      hex
          call      blank
          call      blank
          call      outdi
          inc       di
          call      crlf
          xor       al,al
          jmp short comp

ztrace:
; just like trace except skips OVER next INT or CALL.
          call      setadd                  ; get potential starting point
          call      geteol                  ; check for end of line
          mov       [tcount],1              ; only a single go at it
          mov       es,[cssave]             ; point to instruction to execute
          mov       di,[ipsave]             ; include offset in segment
          xor       dx,dx                   ; where to place breakpoint
          mov       al,es:[di]              ; get the opcode
          cmp       al,11101000b            ; direct intra call
          jz        ztrace3                 ; yes, 3 bytes
          cmp       al,10011010b            ; direct inter call
          jz        ztrace5                 ; yes, 5 bytes
          cmp       al,11111111b            ; indirect?
          jz        ztracemodrm             ; yes, go figure length
          cmp       al,11001100b            ; short interrupt?
          jz        ztrace1                 ; yes, 1 byte
          cmp       al,11001101b            ; long interrupt?
          jz        ztrace2                 ; yes, 2 bytes
          cmp       al,11100010b            ; loop
          jz        ztrace2                 ; 2 byter
          cmp       al,11100001b            ; loopz/loope
          jz        ztrace2                 ; 2 byter
          cmp       al,11100000b            ; loopnz/loopne
          jz        ztrace2                 ; 2 byter
          and       al,11111110b            ; check for rep
          cmp       al,11110010b            ; perhaps?
          jnz       step                    ; can't do anything special, step
          mov       al,es:[di+1]            ; next instruction
          and       al,11111110b            ; ignore w bit
          cmp       al,10100100b            ; MOVS
          jz        ztrace2                 ; two byte
          cmp       al,10100110b            ; CMPS
          jz        ztrace2                 ; two byte
          cmp       al,10101110b            ; SCAS
          jz        ztrace2                 ; two byte
          cmp       al,10101100b            ; LODS
          jz        ztrace2                 ; two byte
          cmp       al,10101010b            ; STOS
          jz        ztrace2                 ; two byte
          jmp       step                    ; bogus, do single step

ztracemodrm:
          mov       al,es:[di+1]            ; get next byte
          and       al,11111000b            ; get mod and type
          cmp       al,01010000b            ; indirect intra 8 bit offset?
          jz        ztrace3                 ; yes, three byte whammy
          cmp       al,01011000b            ; indirect inter 8 bit offset
          jz        ztrace3                 ; yes, three byte guy
          cmp       al,10010000b            ; indirect intra 16 bit offset?
          jz        ztrace4                 ; four byte offset
          cmp       al,10011000b            ; indirect inter 16 bit offset?
          jz        ztrace4                 ; four bytes
          jmp       step                    ; can't figger out what this is!
ztrace5:  inc       dx
ztrace4:  inc       dx
ztrace3:  inc       dx
ztrace2:  inc       dx
ztrace1:  inc       dx
          add       di,dx                   ; offset to breakpoint instruction
          mov       word ptr [bptab],di     ; save offset
          mov       word ptr [bptab+2],es   ; save segment
          mov       al,es:[di]              ; get next opcode byte
          mov       byte ptr [bptab+4],al   ; save it
          mov       byte ptr es:[di],0cch   ; break point it
          mov       [brkcnt],1              ; only this breakpoint
          jmp       dexit                   ; start the operation!

; Trace 1 instruction or the number of instruction specified
; by the parameter using 8086 trace mode. Registers are all
; set according to values in save area

trace:
          call      setadd
          call      scanp
          call      hexin
          mov       dx,1
          jc        stocnt
          mov       cx,4
          call      gethex
stocnt:
          mov       [tcount],dx
          call      geteol
step:
          mov       [brkcnt],0
          or        byte ptr [_fsave+1],1
dexit:
          mov       bx,[user_proc_pdb]
          mov       ah,set_current_pdb
          int       21h
          push      ds
          xor       ax,ax
          mov       ds,ax
          mov       word ptr ds:[12],offset dg:breakfix ; Set vector 3--breakpoint instruction
          mov       word ptr ds:[14],cs
          mov       word ptr ds:[4],offset dg:reenter   ; Set vector 1--Single step
          mov       word ptr ds:[6],cs
          cli

          if        setcntc
          mov       word ptr ds:[8ch],offset dg:contc   ; Set vector 23H (CTRL-C)
          mov       word ptr ds:[8eh],cs
          endif

          pop       ds
          mov       sp,offset dg:stack
          pop       ax
          pop       bx
          pop       cx
          pop       dx
          pop       bp
          pop       bp
          pop       si
          pop       di
          pop       es
          pop       es
          pop       ss
          mov       sp,[spsave]
          push      [_fsave]
          push      [cssave]
          push      [ipsave]
          mov       ds,[dssave]
          iret
step1:
          call      crlf
          call      dispreg
          jmp       short step

; Re-entry point from CTRL-C. Top of stack has address in 86-DOS for
; continuing, so we must pop that off.

contc:
          add       sp,6
          jmp       short reenterreal

; Re-entry point from breakpoint. Need to decrement instruction
; pointer so it points to location where breakpoint actually
; occured.

breakfix:
          push      bp
          mov       bp,sp
          dec       word ptr [bp].oldip
          pop       bp
          jmp       reenterreal

; Re-entry point from trace mode or interrupt during
; execution. All registers are saved so they can be
; displayed or modified.

interrupt_frame struc
oldbp   dw  ?
oldip   dw  ?
oldcs   dw  ?
oldf    dw  ?
olderip dw  ?
oldercs dw  ?
olderf  dw  ?
interrupt_frame ends

reenter:
          push      bp
          mov       bp,sp                   ; get a frame to address from
          push      ax
          mov       ax,cs
          cmp       ax,[bp].oldcs           ; Did we interrupt ourselves?
          jnz       goreenter               ; no, go reenter
          mov       ax,[bp].oldip
          cmp       ax,offset dg:nmiint     ; interrupt below NMI interrupt?
          jb        goreenter               ; yes, go reenter
          cmp       [bp].oldip,offset dg:nmiintend
          jae       goreenter               ; interrupt above NMI interrupt?
          pop       ax                      ; restore state
          pop       bp
          sub       sp,6                    ; switch TRACE and NMI stack frames
          push      bp
          mov       bp,sp                   ; set up frame
          push      ax                      ; get temp variable
          mov       ax,[bp].olderip         ; get NMI Vector
          mov       [bp].oldip,ax           ; stuff in new NMI vector
          mov       ax,[bp].oldercs         ; get NMI Vector
          mov       [bp].oldcs,ax           ; stuff in new NMI vector
          mov       ax,[bp].olderf          ; get NMI Vector
          and       ah,0feh                 ; turn off Trace if present
          mov       [bp].oldf,ax            ; stuff in new NMI vector
          mov       [bp].olderf,ax
          mov       [bp].olderip,offset dg:reenter  ; offset of routine
          mov       [bp].oldercs,cs         ; and cs
          pop       ax
          pop       bp
          iret                              ; go try again
goreenter:
          pop       ax
          pop       bp
reenterreal:
          mov       cs:[spsave+segdif],sp
          mov       cs:[sssave+segdif],ss
          mov       cs:[_fsave],cs
          mov       ss,cs:[_fsave]
          mov       sp,offset dg:rstack
          push      es
          push      ds
          push      di
          push      si
          push      bp
          dec       sp
          dec       sp
          push      dx
          push      cx
          push      bx
          push      ax
          push      ss
          pop       ds
          mov       ss,[sssave]
          mov       sp,[spsave]
          pop       [ipsave]
          pop       [cssave]
          pop       ax
          and       ah,0feh                 ; turn off trace mode bit
          mov       [_fsave],ax
          mov       [spsave],sp
          push      ds
          pop       es
          push      ds
          pop       ss
          mov       sp,offset dg:stack
          push      ds
          xor       ax,ax
          mov       ds,ax

          if        setcntc
          mov       word ptr ds:[8ch],offset dg:dabort  ; Set Ctrl-C vector
          mov       word ptr ds:[8eh],cs
          endif

          pop       ds
          sti
          cld
          mov       ah,get_current_pdb
          int       21h
          mov       [user_proc_pdb],bx
          mov       bx,ds
          mov       ah,set_current_pdb
          int       21h
          dec       [tcount]
          jz        checkdisp
          jmp       step1
checkdisp:
          mov       si,offset dg:bptab
          mov       cx,[brkcnt]
          jcxz      shoreg
          push      es
clearbp:
          les       di,dword ptr [si]
          add       si,4
          movsb
          loop      clearbp
          pop       es
shoreg:
          call      crlf
          call      dispreg
          jmp       command

setadd:
          mov       bp,[cssave]
          call      scanp
          cmp       byte ptr [si],'='
          jnz       ret$5
          inc       si
          call      address
          mov       [cssave],ax
          mov       [ipsave],dx
ret$5:    ret

; Jump to program, setting up registers according to the
; save area. up to 10 breakpoint addresses may be specified.

go:
          call      setadd
          xor       bx,bx
          mov       di,offset dg:bptab
go1:
          call      scanp
          jz        dexec
          mov       bp,[cssave]
          call      address
          mov       [di],dx                 ; Save offset
          mov       [di+2],ax               ; Save segment
          add       di,5                    ; Leave a little room
          inc       bx
          cmp       bx,1+bpmax
          jnz       go1
          mov       ax,5000h+'B'            ; bp error
          jmp       err
dexec:
          mov       [brkcnt],bx
          mov       cx,bx
          jcxz      nobp
          mov       di,offset dg:bptab
          push      ds
setbp:
          lds       si,es:dword ptr [di]
          add       di,4
          movsb
          mov       byte ptr [si-1],0cch
          loop      setbp
          pop       ds
nobp:
          mov       [tcount],1
          jmp       dexit

skip_file:

find_delim:
          lodsb
          call      delim1
          jz        gotdelim
          call      delim2
          jnz       find_delim
gotdelim:
          dec       si
          ret

prepname:
          mov       es,dssave
          push      si
          mov       di,81h
comtail:
          lodsb
          stosb
          cmp       al,13
          jnz       comtail
          sub       di,82h
          xchg      ax,di
          mov       es:(byte ptr [80h]),al
          pop       si
          mov       di,fcb
          mov       ax,(parse_file_descriptor shl 8) or 01h
          int       21h
          mov       byte ptr [axsave],al    ; Indicate analysis of first parm
          call      skip_file
          mov       di,6ch
          mov       ax,(parse_file_descriptor shl 8) or 01h
          int       21h
          mov       byte ptr [axsave+1],al  ; Indicate analysis of second parm
ret23:    ret


;  OPENS A XENIX PATHNAME SPECIFIED IN THE UNFORMATTED PARAMETERS
;  VARIABLE [XNXCMD] SPECIFIES WHICH COMMAND TO OPEN IT WITH
;
;  VARIABLE [HANDLE] CONTAINS THE HANDLE
;  VARIABLE [EXTPTR] POINTS TO THE FILES EXTENSION

delete_a_file:
          mov       byte ptr [xnxcmd],unlink
          jmp       short oc_file

parse_a_file:
          mov       byte ptr [xnxcmd],0
          jmp       short oc_file

exec_a_file:
          mov       byte ptr [xnxcmd],exec
          mov       byte ptr [xnxopt],1
          jmp       short oc_file

open_a_file:
          mov       byte ptr [xnxcmd],open
          mov       byte ptr [xnxopt],2     ; Try read write
          call      oc_file
          jnc       ret23
          mov       byte ptr [xnxcmd],open
          mov       byte ptr [xnxopt],0     ; Try read only
          jmp       short oc_file

create_a_file:
          mov       byte ptr [xnxcmd],creat

oc_file:
          push      ds
          push      es
          push      ax
          push      bx
          push      cx
          push      dx
          push      si
          xor       ax,ax
          mov       [extptr],ax             ; INITIALIZE POINTER TO EXTENSIONS

          mov       si,81h

open1:    call      getchrup
          call      delim2                  ; END OF LINE?
          jz        open4
          call      delim1                  ; SKIP LEADING DELIMITERS
          jz        open1

          mov       dx,si                   ; SAVE POINTER TO BEGINNING
          dec       dx
open2:    cmp       al,'.'                  ; LAST CHAR A "."?
          jnz       open3
          mov       [extptr],si             ; SAVE POINTER TO THE EXTENSION
open3:    call      getchrup
          call      delim1                  ; LOOK FOR END OF PATHNAME
          jz        open4
          call      delim2
          jnz       open2

open4:    dec       si                      ; POINT BACK TO LAST CHAR
          push      [si]                    ; SAVE TERMINATION CHAR
          mov       byte ptr [si],0         ; NULL TERMINATE THE STRING

          mov       al,[xnxopt]
          mov       ah,[xnxcmd]             ; OPEN OR CREATE FILE
          or        ah,ah
          jz        opnret
          mov       bx,offset dg:exec_block
          xor       cx,cx
          int       21h
          mov       cs:[handle],ax          ; SAVE ERROR CODE OR HANDLE

opnret:   pop       [si]

          pop       si
          pop       dx
          pop       cx
          pop       bx
          pop       ax
          pop       es
          pop       ds
          ret

getchrup:
          lodsb
          cmp       al,"a"
          jb        gcur
          cmp       al,"z"
          ja        gcur
          sub       al,32
          mov       [si-1],al
gcur:     ret

delim0:   cmp       al,'['
          jz        limret
delim1:   cmp       al,' '                  ; SKIP THESE GUYS
          jz        limret
          cmp       al,';'
          jz        limret
          cmp       al,'='
          jz        limret
          cmp       al,9
          jz        limret
          cmp       al,','
          jmp       short limret

delim2:   cmp       al,'/'           ; STOP ON THESE GUYS
          jz        limret
          cmp       al,13
limret:   ret

_name:
          call      prepname
          mov       al,byte ptr axsave
          mov       parserr,al
          push      es
          pop       ds
          push      cs
          pop       es
          mov       si,fcb                  ; DS:SI points to user FCB
          mov       di,si                   ; ES:DI points to DEBUG FCB
          mov       cx,82
          rep       movsw
ret6:     ret

badnam:
          mov       dx,offset dg:nambad
          jmp       restart

ifhex:
          cmp       byte ptr [parserr],-1   ; Invalid drive specification?
          jz        badnam
          call      parse_a_file
          mov       bx,[extptr]
          cmp       word ptr ds:[bx],'EH'   ; "HE"
          jnz       ret6
          cmp       byte ptr ds:[bx+2],'X'
          ret

ifexe:
          push      bx
          mov       bx,[extptr]
          cmp       word ptr ds:[bx],'XE'   ; "EX"
          jnz       retif
          cmp       byte ptr ds:[bx+2],'E'
retif:    pop       bx
          ret

load:
          mov       byte ptr [rdflg],read
          jmp       short dskio

dwrite:
          mov       byte ptr [rdflg],write
dskio:
          mov       bp,[cssave]
          call      scanb
          jnz       primio
          jmp       defio
primio:   call      address
          call      scanb
          jnz       prmio
          jmp       fileio
prmio:    push      ax                      ; Save segment
          mov       bx,dx                   ; Put displacement in proper register
          mov       cx,1
          call      gethex                  ; Drive number must be 1 digit
          push      dx
          mov       cx,4
          call      gethex                  ; Logical record number
          push      dx
          mov       cx,3
          call      gethex                  ; Number of records
          call      geteol
          mov       cx,dx
          pop       dx                      ; Logical record number
          pop       ax                      ; Drive number
          cbw                               ; Turn off verify after write
          mov       byte ptr drvlet,al      ; Save drive in case of error
          push      ax
          push      bx
          push      dx
          mov       dl,al
          inc       dl
          mov       ah,get_dpb
          int       21h
          pop       dx
          pop       bx
          or        al,al
          pop       ax
          pop       ds                      ; Segment of transfer
          jnz       drverrj
          cmp       cs:byte ptr [rdflg],write
          jz        abswrt
          int       25h                     ; Primitive disk read
          jmp short endabs

; uso: Some things to note for the fix to support BIGFAT devices based on RBIL
;
; If the call comes back with CY+AX=0207h:
;   set CX=0FFFFh
;   point DS:BX to a struct as follows
;     +0 - DWORD starting sector (first 2 bytes are your old DX)
;     +4 - WORD  sector count (your old CX)
;     +6 - DWORD target address (your old DS:BX).
;
; Not sure how or if we can grab a 32-bit value for our sector number from
; DEBUG's parser.

abswrt:
          int       26h                     ; Primitive disk write
endabs:
          jnc       ret0
drverrj:  jmp       drverr

ret0:
          popf
          ret

defio:
          mov       ax,[cssave]             ; Default segment
          mov       dx,100h                 ; Default file I/O offset
          call      ifhex
          jnz       exechk
          xor       dx,dx                   ; If HEX file, default OFFSET is zero
hex2binj: jmp       hex2bin

fileio:
; AX and DX have segment and offset of transfer, respectively
          call      ifhex
          jz        hex2binj
exechk:
          call      ifexe
          jnz       binfil
          cmp       byte ptr [rdflg],read
          jz        exelj
          mov       dx,offset dg:exewrt
          jmp       restart                 ; Can't write .EXE files

binfil:
          cmp       byte ptr [rdflg],write
          jz        binload
          cmp       word ptr ds:[bx],4f00h+'C'    ; "CO"
          jnz       binload
          cmp       byte ptr ds:[bx+2],'M'
          jnz       binload
exelj:
          dec       si
          cmp       dx,100h
          jnz       prer
          cmp       ax,[cssave]
          jz        oaf
prer:     jmp       perror
oaf:      call      open_a_file
          jnc       gdopen
          mov       ax,exec_file_not_found
          jmp       execerr

gdopen:   xor       dx,dx
          xor       cx,cx
          mov       bx,[handle]
          mov       al,2
          mov       ah,lseek
          int       21h
          call      ifexe                   ; SUBTRACT 512 BYTES FOR EXE
          jnz       bin2                    ; FILE LENGTH BECAUSE OF
          sub       ax,512                  ; THE HEADER
bin2:     mov       [bxsave],dx             ; SET UP FILE SIZE IN DX:AX
          mov       [cxsave],ax
          mov       ah,close
          int       21h
          jmp       exeload

no_mem_err:
          mov       dx,offset dg:toobig
          mov       ah,std_con_string_output
          int       21h
          jmp       command

wrtfilej: jmp   wrtfile
nofilej: jmp    nofile

binload:
          push      ax
          push      dx
          cmp       byte ptr [rdflg],write
          jz        wrtfilej
          call      open_a_file
          jc        nofilej
          mov       bx,[handle]
          mov       ax,(lseek shl 8) or 2
          xor       dx,dx
          mov       cx,dx
          int       21h                     ; GET SIZE OF FILE
          mov       si,dx
          mov       di,ax                   ; SIZE TO SI:DI
          mov       ax,(lseek shl 8) or 0
          xor       dx,dx
          mov       cx,dx
          int       21h                     ; RESET POINTER BACK TO BEGINNING
          pop       ax
          pop       bx
          push      bx
          push      ax                      ; TRANS ADDR TO BX:AX
          add       ax,15
          mov       cl,4
          shr       ax,cl
          add       bx,ax                   ; Start of transfer rounded up to seg
          mov       dx,si
          mov       ax,di                   ; DX:AX is size
          mov       cx,16
          div       cx
          or        dx,dx
          jz        norem
          inc       ax
norem:                                  ; AX is number of paras in transfer
          add       ax,bx                   ; AX is first seg that need not exist
          cmp       ax,cs:[pdb_block_len]
          ja        no_mem_err
          mov       cxsave,di
          mov       bxsave,si
          pop       dx
          pop       ax

rdwr:
; AX:DX is disk transfer address (segment:offset)
; SI:DI is length (32-bit number)

rdwrloop:
          mov       bx,dx                   ; Make a copy of the offset
          and       dx,000fh                ; Establish the offset in 0H-FH range
          mov       cl,4
          shr       bx,cl                   ; Shift offset and
          add       ax,bx                   ; Add to segment register to get new Seg:offset
          push      ax
          push      dx                      ; Save AX,DX register pair
          mov       word ptr [transadd],dx
          mov       word ptr [transadd+2],ax
          mov       cx,0fff0h               ; Keep request in segment
          or        si,si                   ; Need > 64K?
          jnz       bigrdwr
          mov       cx,di                   ; Limit to amount requested
bigrdwr:
          push      ds
          push      bx
          mov       bx,[handle]
          mov       ah,[rdflg]
          lds       dx,[transadd]
          int       21h                     ; Perform read or write
          pop       bx
          pop       ds
          jc        badwr
          cmp       byte ptr [rdflg],write
          jnz       goodr
          cmp       cx,ax
          jz        goodr
badwr:    mov       cx,ax
          stc
          pop       dx                      ; READ OR WRITE BOMBED OUT
          pop       ax
          ret

goodr:
          mov       cx,ax
          sub       di,cx                   ; Request minus amount transferred
          sbb       si,0                    ; Ripple carry
          or        cx,cx                   ; End-of-file?
          pop       dx                      ; Restore DMA address
          pop       ax
          jz        ret8
          add       dx,cx                   ; Bump DMA address by transfer length
          mov       bx,si
          or        bx,di                   ; Finished with request
          jnz       rdwrloop
ret8:     clc                               ; End-of-file not reached
          ret

nofile:
          mov       dx,offset dg:notfnd
restartjmp:
          jmp       restart

wrtfile:
          call      create_a_file           ; Create file we want to write to
          mov       dx,offset dg:noroom     ; Creation error - report error
          jc        restartjmp
          mov       si,bxsave               ; Get high order number of bytes to transfer
          cmp       si,000fh
          jle       wrtsize                 ; Is bx less than or equal to FH
          xor       si,si                   ; Ignore BX if greater than FH - set to zero
wrtsize:
          mov       dx,offset dg:wrtmes1    ; Print number bytes we are writing
          call      rprbuf
          or        si,si
          jz        nxtbyt
          mov       ax,si
          call      digit
nxtbyt:
          mov       dx,cxsave
          mov       di,dx
          call      out16                   ; Amount to write is SI:DI
          mov       dx,offset dg:wrtmes2
          call      rprbuf
          pop       dx
          pop       ax
          call      rdwr
          jnc       clsfle
          call      clsfle
          call      delete_a_file
          mov       dx,offset dg:nospace
          jmp       restartjmp
          call      clsfle
          jmp       command

clsfle:
          mov       ah,close
          mov       bx,[handle]
          int       21h
          ret

exeload:
          pop       [retsave]               ; Suck up return addr
          inc       byte ptr [newexec]
          mov       bx,[user_proc_pdb]
          mov       ax,ds
          cmp       ax,bx
          jz        debug_current
          jmp       find_debug

debug_current:
          mov       ax,[dssave]
debug_found:
          mov       byte ptr [newexec],0
          mov       [headsave],ax
          push      [retsave]               ; Get the return address back
          push      ax
          mov       bx,cs
          sub       ax,bx
          push      cs
          pop       es
          mov       bx,ax
          add       bx,10h                  ; RESERVE HEADER
          mov       ah,setblock
          int       21h
          pop       ax
          mov       word ptr [com_line+2],ax
          mov       word ptr [com_fcb1+2],ax
          mov       word ptr [com_fcb2+2],ax

          call      exec_a_file
          jc        execerr
          call      set_terminate_vector    ; Reset int 22
          mov       ah,get_current_pdb
          int       21h
          mov       [user_proc_pdb],bx
          mov       [dssave],bx
          mov       [essave],bx
          mov       es,bx
          mov       word ptr es:[pdb_exit],offset dg:terminate
          mov       word ptr es:[pdb_exit+2],ds
          les       di,[com_csip]
          mov       [cssave],es
          mov       [ipsave],di
          mov       word ptr [disadd+2],es
          mov       word ptr [disadd],di
          mov       word ptr [asmadd+2],es
          mov       word ptr [asmadd],di
          mov       word ptr [defdump+2],es
          mov       word ptr [defdump],di
          mov       bx,ds
          mov       ah,set_current_pdb
          int       21h
          les       di,[com_sssp]
          mov       ax,es:[di]
          inc       di
          inc       di
          mov       [axsave],ax
          mov       [sssave],es
          mov       [spsave],di
          ret

execerr:
          mov       dx,offset dg:notfnd
          cmp       ax,exec_file_not_found
          jz        gotexecemes
          mov       dx,offset dg:accmes
          cmp       ax,error_access_denied
          jz        gotexecemes
          mov       dx,offset dg:toobig
          cmp       ax,exec_not_enough_memory
          jz        gotexecemes
          mov       dx,offset dg:exebad
          cmp       ax,exec_bad_format
          jz        gotexecemes
          mov       dx,offset dg:execemes
gotexecemes:
          mov       ah,std_con_string_output
          int       21h
          jmp       command

hex2bin:
          mov       [index],dx
          mov       dx,offset dg:hexwrt
          cmp       byte ptr [rdflg],write
          jnz       rdhex
          jmp       restartj2
rdhex:
          mov       es,ax
          call      open_a_file
          mov       dx,offset dg:notfnd
          jnc       hexfnd
          jmp       restart
hexfnd:
          xor       bp,bp
          mov       si,offset dg:(buffer+bufsiz)    ; Flag input buffer as empty
readhex:
          call      getch
          cmp       al,':'                  ; Search for : to start line
          jnz       readhex
          call      getbyt                  ; Get byte count
          mov       cl,al
          mov       ch,0
          jcxz      hexdone
          call      getbyt                  ; Get high byte of load address
          mov       bh,al
          call      getbyt                  ; Get low byte of load address
          mov       bl,al
          add       bx,[index]              ; Add in offset
          mov       di,bx
          call      getbyt                  ; Throw away type byte
readln:
          call      getbyt                  ; Get data byte
          stosb
          cmp       di,bp                   ; Check if this is the largest address so far
          jbe       havbig
          mov       bp,di                   ; Save new largest
havbig:
          loop      readln
          jmp       short readhex

getch:
          cmp       si,offset dg:(buffer+bufsiz)
          jnz       noread
          mov       dx,offset dg:buffer
          mov       si,dx
          mov       ah,read
          push      bx
          push      cx
          mov       cx,bufsiz
          mov       bx,[handle]
          int       21h
          pop       cx
          pop       bx
          or        ax,ax
          jz        hexdone
noread:
          lodsb
          cmp       al,1ah
          jz        hexdone
          or        al,al
          jnz       ret7
hexdone:
          mov       [cxsave],bp
          mov       bxsave,0
          ret

hexdig:
          call      getch
          call      hexchk
          jnc       ret7
          mov       dx,offset dg:hexerr
restartj2:
          jmp       restart

getbyt:
          call      hexdig
          mov       bl,al
          call      hexdig
          shl       bl,1
          shl       bl,1
          shl       bl,1
          shl       bl,1
          or        al,bl
ret7:     ret


code      ends
          end
