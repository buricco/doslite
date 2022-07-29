; Copyright (C) 1983 Microsoft Corp.
; Copyright     2022 S. V. Nickolas.
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

false     equ       0
true      equ       not false

fcb       equ       5Ch

quotchr   equ       16h                 ; quote character = ^V

include   dossym.inc

code      segment   public
code      ends

const     segment   public word
          extrn     txt1:byte,txt2:byte,fudge:byte,hardch:dword,userdir:byte
const     ends

data      segment   public word
          extrn     oldlen:word,olddat:byte,srchflg:byte,comline:word
          extrn     param1:word,param2:word,newlen:word,srchmod:byte
          extrn     current:word,pointer:word,start:byte,endtxt:word
          extrn     usrdrv:byte,lstnum:word,numpos:word,lstfnd:word
          extrn     srchcnt:word
data      ends

dg        group     code,const,data

code      segment   public

assume    cs:dg,ds:dg,ss:dg,es:dg

          public    rest_dir,kill_bl,int_24,scanln,findlin,shownum
          public    fndfirst,fndnext,crlf,lf,_out,unquote
          extrn     chkrange:near

edlproc:
ret1:     ret
fndfirst: mov       di,1+offset dg:txt1
          mov       byte ptr [olddat],1 ; replace with old value if none new
          call      gettext
          or        al,al               ; Reset zero flag in case CX is zero
          jcxz      ret1
          cmp       al,1Ah              ; terminated with a ^z ?
          jne       sj8
          mov       byte ptr [olddat],0 ; do not replace with old value
sj8:      mov       [oldlen],cx
          xor       cx,cx
          cmp       al,0Dh
          jz        setbuf
          cmp       byte ptr [srchflg],0
          jz        nxtbuf
setbuf:   dec       si
nxtbuf:   mov       [comline],si
          mov       di,1+offset dg:txt2
          call      gettext
          cmp       byte ptr [srchflg],0
          jnz       notrepl
          cmp       al,0Dh
          jnz       havchr
          dec       si
havchr:   mov       [comline],si
notrepl:  mov       [newlen],cx
          mov       bx,[param1]
          or        bx,bx
          jnz       caller
          cmp       byte ptr[srchmod],0
          jne       sj9
          mov       bx,1                ; start from line number 1
          jmp       short sj9a
sj9:      mov       bx,[current]
          inc       bx                  ; Default search/replace to current+1
sj9a:     call      chkrange
caller:   call      findlin
          mov       [lstfnd],di
          mov       [numpos],di
          mov       [lstnum],dx
          mov       bx,[param2]
          cmp       bx,1
          sbb       bx,-1               ; Decrement everything except zero
          call      findlin
          mov       cx,di
          sub       cx,[lstfnd]
          or        al,-1
          jcxz      aret
          cmp       cx,[oldlen]
          jae       sj10
aret:     ret
sj10:     mov       [srchcnt],cx

; Inputs:
;       [TXT1+1] has string to search for
;       [OLDLEN] has length of the string
;       [LSTFND] has starting position of search in text buffer
;       [LSTNUM] has line number which has [LSTFND]
;       [SRCHCNT] has length to be searched
;       [NUMPOS] has beginning of line which has [LSTFND]
; Outputs:
;       Zero flag set if match found
;       [LSTFND],[LSTNUM],[SRCHCNT] updated for continuing the search
;       [NUMPOS] has beginning of line in which match was made

fndnext:  mov       al,[txt1+1]
          mov       cx,[srchcnt]
          mov       di,[lstfnd]
scan:     or        di,di               ; Clear zero flag in case CX=0
          repne     scasb
          jnz       ret11
          mov       dx,cx
          mov       bx,di               ; Save search position
          mov       cx,[oldlen]
          dec       cx
          mov       si, offset dg:txt1+2
          cmp       al,al               ; Set zero flag in case CX=0
          repe      cmpsb
          mov       cx,dx
          mov       di,bx
          jnz       scan
          mov       [srchcnt],cx
          mov       cx,di
          mov       [lstfnd],di
          mov       di,[numpos]
          sub       cx,di
          mov       al,10
          mov       dx,[lstnum]
getlin:   inc       dx                  ; Determine line number of match
          mov       bx,di
          repne     scasb
          jz        getlin
          dec       dx
          mov       [lstnum],dx
          mov       [numpos],bx
          xor       al,al
ret11:    ret

; Inputs:
;       SI points into command line buffer
;       DI points to result buffer
; Function:
;       Moves [SI] to [DI] until ctrl-Z (1AH) or
;       RETURN (0DH) is found. Termination char not moved.
; Outputs:
;       AL = Termination character
;       CX = No of characters moved.
;       SI points one past termination character
;       DI points to next free location

gettext:  xor       cx,cx
getit:    lodsb
;-----------------------------------------------------------------------
          cmp       al,quotchr          ; a quote character?
          jne       sj101               ; no, skip....
          lodsb                         ; yes, get quoted character
          call      makectrl
          jmp short sj102
;-----------------------------------------------------------------------
sj101:    cmp       al,1Ah
          jz        defchk
sj102:    cmp       al,0Dh
          jz        defchk
          stosb
          inc       cx
          jmp       short getit
defchk:   or        cx,cx
          jz        oldtxt
          push      di
          sub       di,cx
          mov       byte ptr [di-1],cl
          pop       di
          ret
oldtxt:   cmp       byte ptr [olddat],1 ; replace with old text?
          je        sj11                ; yes...
          mov       byte ptr [di-1],cl  ; zero text buffer char count
          ret
sj11:     mov       cl,byte ptr [di-1]
          add       di,cx
          ret

; Inputs
;       BX = Line number to be located in buffer (0 means last line)
; Outputs:
;       DX = Actual line found
;       DI = Pointer to start of line DX
;       Zero set if BX = DX
; AL,CX destroyed. No other registers affected.

findlin:  mov       dx,[current]
          mov       di,[pointer]
          cmp       bx,dx
          jz        ret4
          ja        findit
          or        bx,bx
          jz        findit
          mov       dx,1
          mov       di,offset dg:start
          cmp       bx,dx
          jz        ret4
findit:   mov       cx,[endtxt]
          sub       cx,di
scanln:   mov       al,10
          or        al,al               ; Clear zero flag
finlin:   jcxz      ret4
          repne     scasb
          inc       dx
          cmp       bx,dx
          jnz       finlin
ret4:     ret

; Inputs:
;       BX = Line number to be displayed
; Function:
;       Displays line number on terminal in 8-character
;       format, suppressing leading zeros.
; AX, CX, DX destroyed. No other registers affected.

shownum:  push      bx
          mov       al," "
          call      _out
          call      conv10
          mov       al,":"
          call      _out
          mov       al,"*"
          pop       bx
          cmp       bx,[current]
          jz        starlin
          mov       al," "
starlin:  jmp       _out

;Inputs:
;       BX = Binary number to be displayed
; Function:
;       Ouputs binary number. Five digits with leading
;       zero suppression. Zero prints 5 blanks.

conv10:   xor       ax,ax
          mov       dl,al
          mov       cx,16
conv:     shl       bx,1
          adc       al,al
          daa
          xchg      al,ah
          adc       al,al
          daa
          xchg      al,ah
          adc       dl,dl
          loop      conv
          mov       bl,'0'-' '
          xchg      ax,dx
          call      ldig
          mov       al,dh
          call      digits
          mov       al,dl
digits:   mov       dh,al
          shr       al,1
          shr       al,1
          shr       al,1
          shr       al,1
          call      ldig
          mov       al,dh
ldig:     and       al,0Fh
          jz        zerdig
          mov       bl,0
zerdig:   add       al,"0"
          sub       al,bl
          jmp       _out
ret5:     ret
crlf:     mov       al,13
          call      _out
lf:       mov       al,10
_out:     push      dx
          xchg      ax,dx
          mov       ah,std_con_output
          int       21h
          xchg      ax,dx
          pop       dx
          ret

;-----------------------------------------------------------------------;
; Will scan buffer given pointed to by SI and get rid of quote
;characters, compressing the line and adjusting the length at the
;begining of the line.
; Preserves al registers except flags and AX .

unquote:  push      cx
          push      di
          push      si
          mov       di,si
          mov       cl,[si-1]           ; length of buffer
          xor       ch,ch
          mov       al,quotchr
          cld
unq_loop: jcxz      unq_done            ; no more chars in the buffer, exit
          repnz scasb                   ; search for quote character
          jnz       unq_done            ; none found, exit
          push      cx                  ; save chars left in buffer
          push      di                  ; save pointer to quoted character
          push      ax                  ; save quote character
          mov       al,byte ptr [di]    ; get quoted character
          call      makectrl
          mov       byte ptr [di],al
          pop       ax                  ; restore quote character
          mov       si,di
          dec       di                  ; points to the quote character
          inc       cx                  ; include the carriage return also
          rep movsb                     ; compact line
          pop       di                  ; now points to after quoted character
          pop       cx
          jcxz      sj13                ; if quote char was last of line
                                        ; do not adjust
          dec       cx                  ; one less char left in the buffer
sj13:     pop     si
          dec       byte ptr [si-1]     ; one less character in total buffer 
          push      si                  ; count also
          jmp short unq_loop
unq_done: pop       si
          pop       di
          pop       cx
          ret

;-----------------------------------------------------------------------;
;       Convert the character in AL to the corresponding control
; character. AL has to be between @ and _ to be converted. That is,
; it has to be a capital letter. All other letters are left unchanged.

makectrl: push      ax
          and       ax,11100000b
          cmp       ax,01000000b
          pop       ax
          jne       sj14
          and       ax,00011111b
sj14:     ret

;---- Kill spaces in buffer --------------------------------------------;
kill_bl:  lodsb                         ; get rid of blanks
          cmp       al,' '
          je        kill_bl
          ret

;----- Restore INT 24 vector and old current directory -----------------;
rest_dir: cmp       [fudge],0
          je        no_fudge
          mov       ax,(set_interrupt_vector shl 8) or 24h
          lds       dx,[hardch]
          int       21h
          push      cs
          pop       ds
          mov       dx,offset dg:userdir
          mov       ah,chdir
          int       21h
          mov       dl,[usrdrv]         ; restore old current drive
          mov       ah,set_default_drive
          int       21h
no_fudge: ret

;----- INT 24 Processing -----------------------------------------------;

int24ret: dw       offset dg:int24bak

int_24    proc    far
assume    ds:nothing,es:nothing,ss:nothing
          pushf
          push      cs
          push      [int24ret]
          push      word ptr [hardch+2]
          push      word ptr [hardch]
          ret
int_24    endp

int24bak: cmp       al,2                ; abort?
          jnz       ireti
          push      cs
          pop       ds

assume    ds:dg

          call      rest_dir
          int       20h
ireti:    iret

;-----------------------------------------------------------------------;

code      ends
          end
