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

; DEBUG-86 8086 debugger runs under 86-DOS       version 2.30
;
; Modified 5/4/82 by AaronR to do all I/O direct to devices
; Runs on MS-DOS 1.28 and above
; REV 1.20
;       Tab expansion
;       New device interface (1.29 and above)
; REV 2.0
;       line by line assembler added by C. Peters
; REV 2.1
;       Uses EXEC system call
; REV 2.2
;       Ztrace mode by zibo.
;       Fix dump display to indent properly
;       Parity nonsense by zibo
;
; REV 2.3
;       Split into seperate modules to allow for
;       assembly on an IBM PC
;

          include   debequ.inc
          include   dossym.inc

code      segment   public 'code'
code      ends

const     segment   public byte

          extrn     user_proc_pdb:word,stack:byte,cssave:word,dssave:word
          extrn     spsave:word,ipsave:word,linebuf:byte,qflag:byte
          extrn     newexec:byte,headsave:word,lbufsiz:byte,bacmes:byte
          extrn     badver:byte,endmes:byte,carret:byte,ptymes:byte

ifdef     help                          ; uso.
          extrn     helpmsg:byte,olhelp:byte
endif

          extrn     dsiz:byte,noregl:byte,dispb:word

const     ends

data      segment   public byte

          extrn     parserr:byte,dataend:word,ptyflag:byte,disadd:byte
          extrn     asmadd:byte,defdump:byte,bytebuf:byte

data      ends

dg        group     code,const,data


code      segment   public 'code'
assume    cs:dg,ds:dg,es:dg,ss:dg

          public    restart,set_terminate_vector,dabort,terminate,command
          public    find_debug,crlf,blank,tab,_out,inbuf,scanb,scanp
          public    printmes,rprbuf,hex,outsi,outdi,out16,digit,backup,rbufin

          extrn     perr:near,compare:near,dump:near,_enter:near,fill:near
          extrn     go:near,input:near,load:near,move:near,_name:near
          extrn     reg:near,search:near,dwrite:near,unassem:near,assem:near
          extrn     output:near,ztrace:near,trace:near,gethex:near,geteol:near

          extrn     prepname:near,defio:near,skip_file:near,debug_found:near
          extrn     trapparity:near,releaseparity:near

          org       100h

start:
debug:    jmp short dstrt

header:   db        "DEBUG v2.30-je1-watcom-uso2"
          db        13,10,'$'

dstrt:    mov       ah,get_version
          int       21h
          xchg      ah,al               ; Turn it around to AH.AL
          cmp       ax, 0200h           ; MS-DOS 2.0+?
          jae       okdos
          mov       dx,offset dg:badver
          mov       ah,std_con_string_output
          int       21h
          int       20h

okdos:
ifdef     help                          ; uso.
          mov       si, 81h
help1:    cmp       byte ptr [si], ' '
          jne       help3
help2:    inc       si
          jmp short help1
help3:    cmp       byte ptr [si], 9
          je        help2
          cmp       word ptr [si], '?/'
          jne       nohelp
          mov       ah, std_con_string_output
          mov       dx, offset dg:helpmsg
          int       21h
          int       20h
nohelp:
endif
          call      trapparity          ; scarf up those parity guys
          mov       ah,get_current_pdb
          int       21h
          mov       [user_proc_pdb],bx  ; Initially set to DEBUG
          mov       sp,offset dg:stack
          mov       [parserr],al
          mov       ah,get_in_vars
          int       21h
gotlist:  mov       ax,cs
          mov       ds,ax
          mov       es,ax
          mov       dx,offset dg:header ; Code to print header
          call      rprbuf
          call      set_terminate_vector

if        setcntc
          mov       al,23h              ; Set vector 23H
          mov       dx,offset dg:dabort
          int       21h
endif

          mov       dx,cs               ; Get DEBUG's segment
          mov       ax,offset dg:dataend+15
          shr       ax,1                ; Convert to segments
          shr       ax,1
          shr       ax,1
          shr       ax,1
          add       dx,ax               ; Add siz of debug in paragraphs
          mov       ah,create_process_data_block
          int       21h
          mov       ax,dx
          mov       di,offset dg:dssave
          cld
          stosw
          stosw
          stosw
          stosw
          mov       word ptr [disadd+2],ax
          mov       word ptr [asmadd+2],ax
          mov       word ptr [defdump+2],ax
          mov       ax,100h
          mov       word ptr [disadd],ax
          mov       word ptr [asmadd],ax
          mov       word ptr [defdump],ax
          mov       ds,dx
          mov       es,dx
          mov       dx,80h
          mov       ah,set_dma
          int       21h                 ; Set default DMA address to 80H
          mov       ax,word ptr ds:[6]
          mov       bx,ax
          cmp       ax,0fff0h
          push      cs
          pop       ds
          jae       savstk
          mov       ax,word ptr ds:[6]
          push      bx
          mov       bx,offset dg:dataend+15
          and       bx,0fff0h           ; Size of DEBUG in bytes (rounded up to PARA)
          sub       ax,bx
          pop       bx
savstk:   push      bx                  ; Creating a stack at the top of the segment,
          dec       ax                  ; with 0 on the top of the stack.
          dec       ax
          mov       bx,ax
          mov       word ptr [bx],0
          pop       bx
          mov       spsave,ax   
          dec       ah                  ; Allow 128 words of stack
          inc       ax                  ; [JCE] Correction, 127 words. This fixes the 
          inc       ax                  ; bug in CALL 5 that's present in DOS 2.0
          mov       es:word ptr [6],ax  ; all the way to Windows 10
          sub       bx,ax
          mov       cl,4
          shr       bx,cl
          add       es:word ptr [8],bx
          mov       ah,15               ; Get screen size and initialize display related variables
          int       10h
          cmp       ah,40
          jnz       parschk
          mov       byte ptr dsiz,7
          mov       byte ptr noregl,4
          mov       dispb,64

parschk:  mov       di,fcb              ; Copy rest of command line to test program's parameter area
          mov       si,81h
          mov       ax,(parse_file_descriptor shl 8) or 01h
          int       21h
          call      skip_file           ; Make sure si points to delimiter
          call      prepname
          push      cs
          pop       es
filechk:  mov       di,80h
          cmp       byte ptr es:[di],0  ; ANY STUFF FOUND?
          jz        command             ; NOPE
filoop:   inc       di
          cmp       byte ptr es:[di],13 ; COMMAND LINE JUST SPACES?
          jz        command
          cmp       byte ptr es:[di],' '
          jz        filoop
          cmp       byte ptr es:[di],9
          jz        filoop
          call      defio               ; WELL READ IT IN
          mov       ax,cssave
          mov       word ptr disadd+2,ax
          mov       word ptr asmadd+2,ax
          mov       ax,ipsave
          mov       word ptr disadd,ax
          mov       word ptr asmadd,ax
command:  cld
          mov       ax,cs
          mov       ds,ax
          mov       es,ax
          mov       ss,ax
          mov       sp,offset dg:stack
          sti
          cmp       [ptyflag],0         ; did we detect a parity error?
          jz        goprompt            ; nope, go prompt
          mov       [ptyflag],0         ; reset flag
          mov       dx,offset dg:ptymes ; message to print
          mov       ah,std_con_string_output
          int       21h                 ; blam
goprompt: mov       al,prompt
          call      _out
          call      inbuf               ; Get command line

; From now and throughout command line processing, DI points
; to next character in command line to be processed.

          call      scanb               ; Scan off leading blanks
          jz        command             ; Null command?
          lodsb                         ; AL=first non-blank character

; Prepare command letter for table lookup

ifdef     help                          ; uso
          cmp       al,'?'
          jne       nothelp
          mov       dx, offset dg:olhelp
          mov       ah, std_con_string_output
          int       21h
          jmp short goprompt
nothelp:
endif
          sub       al,'A'              ; Low end range check
          jb        err1
          cmp       al,'Z'-'A'          ; Upper end range check
          ja        err1
          shl       al,1                ; Times two
          cbw                           ; Now a 16-bit quantity
          xchg      bx,ax               ; In BX we can address with it
          call      cs:[bx+comtab]      ; Execute command
          jmp       short command       ; Get next command
err1:     jmp       perr

set_terminate_vector:
          mov       ax,(set_interrupt_vector shl 8) or 22h  ; Set vector 22H
          mov       dx,offset dg:terminate
          int       21h
          ret

terminate:
          cmp       byte ptr cs:[qflag],0
          jnz       quiting
          mov       cs:[user_proc_pdb],cs
          cmp       byte ptr cs:[newexec],0
          jz        normterm
          mov       ax,cs
          mov       ds,ax
          mov       ss,ax
          mov       sp,offset dg:stack
          mov       ax,[headsave]
          jmp       debug_found
normterm: mov       dx,offset dg:endmes
          jmp       short restart
quiting:  mov       ax,(exit shl 8)
          int       21h
dabort:   mov       dx,offset dg:carret
restart:  mov       ax,cs
          mov       ds,ax
          mov       ss,ax
          mov       sp,offset dg:stack
          call      rprbuf
          jmp       command

; Get input line. Convert all characters NOT in quotes to upper case.

inbuf:    call      rbufin
          mov       si,offset dg:linebuf
          mov       di,offset dg:bytebuf
casechk:  lodsb
          cmp       al,'a'
          jb        noconv
          cmp       al,'z'
          ja        noconv
          add       al,'A'-'a'          ; Convert to upper case
noconv:   stosb
          cmp       al,13
          jz        indone
          cmp       al,'"'
          jz        quotscan
          cmp       al,"'"
          jnz       casechk
quotscan: mov       ah,al
killstr:  lodsb
          stosb
          cmp       al,13
          jz        indone
          cmp       al,ah
          jnz       killstr
          jmp short casechk
indone:   mov       si,offset dg:bytebuf
crlf:     mov       al,13               ; Output CR/LF sequence
          call      _out
          mov       al,10
          jmp       _out
backup:   mov       si,offset dg:bacmes ; Physical backspace - blank, backspace, blank

; Print ASCII message. Last char has bit 7 set

printmes: lods      cs:byte ptr [si]    ; Get char to print
          call      _out
          shl       al,1                ; High bit set?
          jnc       printmes
          ret

; Scan for parameters of a command

scanp:    call      scanb               ; Get first non-blank
          cmp       byte ptr [si],','   ; One comma between params OK
          jne       eolchk              ; If not comma, we found param
          inc       si                  ; Skip over comma

; Scan command line for next non-blank character

scanb:    push      ax
scannext: lodsb
          cmp       al,' '
          jz        scannext
          cmp       al,9
          jz        scannext
          dec       si                  ; Back to first non-blank
          pop       ax
eolchk:   cmp       byte ptr [si],13
          ret

; Hex addition and subtraction

hexadd:   mov       cx,4
          call      gethex
          mov       di,dx
          mov       cx,4
          call      gethex
          call      geteol
          push      dx
          add       dx,di
          call      out16
          call      blank
          call      blank
          pop       dx
          sub       di,dx
          mov       dx,di
          call      out16
          jmp short crlf

; Print the hex address of DS:SI

outsi:    mov       dx,ds               ; Put DS where we can work with it
          call      out16               ; Display segment
          mov       al,':'
          call      _out
          mov       dx,si
          jmp short out16               ; Output displacement

; Print hex address of ES:DI
; Same as OUTSI above

outdi:    mov       dx,es
          call      out16
          mov       al,':'
          call      _out
          mov       dx,di

; Print out 16-bit value in DX in hex

out16:    mov       al,dh               ; High-order byte first
          call      hex
          mov       al,dl               ; Then low-order byte

; Output byte in AL as two hex digits

hex:      mov       ah,al               ; Save for second digit
          push      cx                  ; Shift high digit into low 4 bits
          mov       cl,4
          shr       al,cl
          pop       cx
          call      digit               ; Output first digit
          mov       al,ah               ; Now do digit saved in AH
digit:    and       al,0fh              ; Mask to 4 bits
          add       al,90h              ; Trick 6-byte hex conversion works
          daa                           ;   on 8086 too.
          adc       al,40h
          daa

; Console output of character in AL. No registers affected but bit 7
; is reset before output.

_out:     push      dx
          push      ax
          and       al,7fh
          mov       dl,al
          mov       ah,2
          int       21h
          pop       ax
          pop       dx
          ret

rbufin:   push      ax
          push      dx
          mov       ah,10
          mov       dx,offset dg:lbufsiz
          int       21h
          pop       dx
          pop       ax
          ret

rprbuf:   mov       ah,9
          int       21h
          ret

blank:    mov       al," "              ; Output one space
          jmp       _out

tab:      call      blank               ; Output the number of blanks in CX
          loop      tab
          ret

; Command Table. Command letter indexes into table to get
; address of command. PERR prints error for no such command.

comtab:   dw        assem               ; A
          dw        perr                ; B
          dw        compare             ; C
          dw        dump                ; D
          dw        _enter              ; E
          dw        fill                ; F
          dw        go                  ; G
          dw        hexadd              ; H
          dw        input               ; I
          dw        perr                ; J
          dw        perr                ; K
          dw        load                ; L
          dw        move                ; M
          dw        _name               ; N
          dw        output              ; O
          dw        ztrace              ; P
          dw        quit                ; Q (Quit)
          dw        reg                 ; R
          dw        search              ; S
          dw        trace               ; T
          dw        unassem             ; U
          dw        perr                ; V
          dw        dwrite              ; W
          dw        perr                ; X - uso will use for EMS
          dw        perr                ; Y
          dw        perr                ; Z

quit:     inc       byte ptr [qflag]
          mov       bx,[user_proc_pdb]
find_debug:
          mov       ah,set_current_pdb
          int       21h
          call      releaseparity       ; let system do normal parity stuff
          mov       ax,(exit shl 8)
          int       21h

code      ends
          end       start
