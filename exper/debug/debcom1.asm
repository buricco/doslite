; Copyright (C) 1983 Microsoft Corp.
; Modifications copyright 2018 John Elliott
;           and copyright 2022 S. V. Nickolas.
; Additional modifications adapted from modifications by C. Masloch
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
          extrn     synerr:byte
          extrn     dispb:word,dsiz:byte,dssave:word
const     ends

data      segment   public byte
          extrn     deflen:word,bytebuf:byte,defdump:byte,error_handler:word
data      ends

dg        group     code,const,data

code      segment public byte 'code'
assume    cs:dg,ds:dg,es:dg,ss:dg

          public    hexchk,gethex1,print,dsrange,address,hexin,perror
          public    gethex,get_address,geteol,gethx,perr
          public    perr,move,dump,_enter,fill,search,default,jump_error
          extrn     _out:near,crlf:near,outdi:near,outsi:near,scanp:near
          extrn     scanb:near,blank:near,tab:near,printmes:near,command:near
          extrn     hex:near,backup:near

debcom1:

; RANGE - Looks for parameters defining an address range.
; The first parameter is the starting address. The second parameter
; may specify the ending address, or it may be preceded by
; "L" and specify a length (4 digits max), or it may be
; omitted and a length of 128 bytes is assumed. Returns with
; segment in AX, displacement in DX, and length in CX.

dsrange:  mov       bp,[dssave]         ; Set default segment to DS
          mov       [deflen],128        ; And default length to 128 bytes
range:    call      address
          push      ax                  ; Save segment
          push      dx                  ; Save offset
          call      scanp               ; Get to next parameter
          mov       al,[si]
          cmp       al,'L'              ; Length indicator?
          je        getlen
          mov       dx,[deflen]         ; Default length
          call      hexin               ; Second parameter present?
          jc        getdef              ; If not, use default
          mov       cx,4
          call      gethex              ; Get ending address (same segment)
          mov       cx,dx               ; Low 16 bits of ending addr.
          pop       dx                  ; Low 16 bits of starting addr.
          sub       cx,dx               ; Compute range
          jae       dsrng2
dsrng1:   jmp       jump_error          ; Negative range
dsrng2:   inc       cx                  ; Include last location
          jcxz      dsrng1              ; Wrap around error
          pop       ax                  ; Restore segment
          ret
getdef:   pop       cx                  ; get original offset
          push      cx                  ; save it
          neg       cx                  ; rest of segment
          jz        rngret              ; use default
          cmp       cx,dx               ; more room in segment?
          jae       rngret              ; yes, use default
          jmp       rngret1             ; no, length is in CX
getlen:   inc       si                  ; Skip over "L" to length
          mov       cx,4                ; Length may have 4 digits
          call      gethex              ; Get the range
rngret:   mov       cx,dx               ; Length
rngret1:  pop       dx                  ; Offset
          mov       ax,cx
          add       ax,dx
          jnc       okret
          cmp       ax,1
          jae       dsrng1              ; Look for wrap error
okret:    pop       ax                  ; Segment
          ret

; DI points to default address and CX has default length

default:  call      scanp
          jz        usedef              ; Use default if no parameters
          mov       [deflen],cx
          call      range
          jmp       geteol
usedef:   mov       si,di
          lodsw                         ; Get default displacement
          mov       dx,ax
          lodsw                         ; Get default segment
          ret

; Dump an area of memory in both hex and ASCII

dump:     mov       bp,[dssave]
          mov       cx,dispb
          mov       di,offset dg:defdump
          call      default             ; Get range if specified
          mov       ds,ax               ; Set segment
          mov       si,dx               ; SI has displacement in segment
          push      si                  ; save SI away
          and       si,0fff0h           ; convert to para number
          call      outsi               ; display location
          pop       si                  ; get SI back
          mov       ax,si               ; move offset
          mov       ah,3                ; spaces per byte
          and       al,0fh              ; convert to real offset
          mul       ah                  ; compute (AL+1)*3-1
          or        al,al               ; set flag
          jz        inrow               ; if zero go on
          push      cx                  ; save count
          mov       cx,ax               ; move to convenient spot
          call      tab                 ; move over
          pop       cx                  ; get back count
          jmp       inrow               ; display line
row:      call      outsi               ; Print address at start of line
inrow:    push      si                  ; Save address for ASCII dump
          call      blank
byte0:    call      blank               ; Space between bytes
byte1:    lodsb                         ; Get byte to dump
          call      hex                 ; and display it
          pop       dx                  ; DX has start addr. for ASCII dump
          dec       cx                  ; Drop loop count
          jz        toascii             ; If through do ASCII dump
          mov       ax,si
          test      al,cs:(byte ptr dsiz)
          jz        endrow
          push      dx                  ; Didn't need ASCII addr. yet
          test      al,7                ; On 8-byte boundary?
          jnz       byte0
          mov       al,'-'              ; Mark every 8 bytes
          call      _out
          jmp       short byte1
endrow:   call      ascii               ; Show it in ASCII
          jmp short row                 ; Loop until count is zero
toascii:  mov       ax,si               ; get offset
          and       al,0fh              ; real offset
          jz        ascii               ; no loop if already there
          sub       al,10h              ; remainder
          neg       al
          mov       cl,3
          mul       cl
          mov       cx,ax               ; number of chars to move
          call      tab
ascii:    push      cx                  ; Save byte count
          mov       ax,si               ; Current dump address
          mov       si,dx               ; ASCII dump address
          sub       ax,dx               ; AX=length of ASCII dump
          mov       cx,si               ; get starting point
          dec       cx
          and       cx,0fh
          inc       cx
          and       cx,0fh
          add       cx,3                ; we have the correct number to tab
          push      ax                  ; save count
          call      tab
          pop       cx                  ; get count back
ascdmp:   lodsb                         ; Get ASCII byte to dump
          and       al,7fh              ; ASCII uses 7 bits
          cmp       al,7fh              ; Don't try to print RUBOUT
          jz        noprt
          cmp       al,' '              ; Check for control characters
          jnc       prin
noprt:    mov       al,'.'              ; If unprintable character
prin:     call      _out                ; Print ASCII character
          loop      ascdmp              ; CX times
          pop       cx                  ; Restore overall dump length
          mov       es:word ptr [defdump],si
          mov       es:word ptr [defdump+2],ds
          call      crlf                ; Print CR/LF and return
          ret

; Block move one area of memory to another. Overlapping moves
; are performed correctly, i.e., so that a source byte is not
; overwritten until after it has been moved.

move:     call      dsrange             ; Get range of source area
          push      cx                  ; Save length
          push      ax                  ; Save segment
          push      dx                  ; Save source displacement
          call      address             ; Get destination address (same segment)
          call      geteol              ; Check for errors
          pop       si
          mov       di,dx               ; Set dest. displacement
          pop       bx                  ; Source segment
          mov       ds,bx
          mov       es,ax               ; Destination segment
          pop       cx                  ; Length
          cmp       di,si               ; Check direction of move
          sbb       ax,bx               ; Extend the CMP to 32 bits
          jb        copylist            ; Move forward into lower mem.

; Otherwise, move backward. Figure end of source and destination
; areas and flip direction flag.

          dec       cx
          add       si,cx               ; End of source area
          add       di,cx               ; End of destination area
          std                           ; Reverse direction
          inc       cx
copylist: movsb                         ; Do at least 1 - Range is 1-10000H not 0-FFFFH
          dec       cx
          rep       movsb               ; Block move
ret1:     ret

; Fill an area of memory with a list values. If the list
; is bigger than the area, don't use the whole list. If the
; list is smaller, repeat it as many times as necessary.

fill:     call      dsrange             ; Get range to fill
          push      cx                  ; Save length
          push      ax                  ; Save segment number
          push      dx                  ; Save displacement
          call      list                ; Get list of values to fill with
          pop       di                  ; Displacement in segment
          pop       es                  ; Segment
          pop       cx                  ; Length
          cmp       bx,cx               ; BX is length of fill list
          mov       si,offset dg:bytebuf
          jcxz      bigrng
          jae       copylist            ; If list is big, copy part of it
bigrng:   sub       cx,bx               ; How much bigger is area than list?
          xchg      cx,bx               ; CX=length of list
          push      di                  ; Save starting addr. of area
          rep       movsb               ; Move list into area
          pop       si

; The list has been copied into the beginning of the
; specified area of memory. SI is the first address
; of that area, DI is the end of the copy of the list
; plus one, which is where the list will begin to repeat.
; All we need to do now is copy [SI] to [DI] until the
; end of the memory area is reached. This will cause the
; list to repeat as many times as necessary.

          mov       cx,bx               ; Length of area minus list
          push      es                  ; Different index register
          pop       ds                  ; requires different segment reg.
          jmp short copylist            ; Do the block move

; Search a specified area of memory for given list of bytes.
; Print address of first byte of each match.

search:   call      dsrange             ; Get area to be searched
          push      cx                  ; Save count
          push      ax                  ; Save segment number
          push      dx                  ; Save displacement
          call      list                ; Get search list
          dec       bx                  ; No. of bytes in list-1
          pop       di                  ; Displacement within segment
          pop       es                  ; Segment
          pop       cx                  ; Length to be searched
          sub       cx,bx               ;  minus length of list
scan:     mov       si,offset dg:bytebuf
          lodsb                         ; Bring first byte into AL
doscan:   scasb                         ; Search for first byte
          loopne    doscan              ; Do at least once by using LOOP
          jnz       ret1                ; Exit if not found
          push      bx                  ; Length of list minus 1
          xchg      bx,cx
          push      di                  ; Will resume search here
          repe      cmpsb               ; Compare rest of string
          mov       cx,bx               ; Area length back in CX
          pop       di                  ; Next search location
          pop       bx                  ; Restore list length
          jnz       _test               ; Continue search if no match
          dec       di                  ; Match address
          call      outdi               ; Print it
          inc       di                  ; Restore search address
          call      crlf
_test:    jcxz      ret1
          jmp short scan                ; Look for next occurrence

; Get the next parameter, which must be a hex number.
; CX is maximum number of digits the number may have.

gethx:    call      scanp
gethx1:   xor       dx,dx               ; Initialize the number
          call      hexin               ; Get a hex digit
          jc        hxerr               ; Must be one valid digit
          mov       dl,al               ; First 4 bits in position
getlp:    inc       si                  ; Next char in buffer
          dec       cx                  ; Digit count
          call      hexin               ; Get another hex digit?
          jc        rethx               ; All done if no more digits
          stc
          jcxz      hxerr               ; Too many digits?
          shl       dx,1                ; Multiply by 16
          shl       dx,1
          shl       dx,1
          shl       dx,1
          or        dl,al               ; and combine new digit
          jmp short getlp               ; Get more digits
gethex:   call      gethx               ; Scan to next parameter
          jmp short gethx2
gethex1:  call      gethx1
gethx2:   jnc       rethx
          jmp       jump_error
rethx:    clc
hxerr:    ret

; Check if next character in the input buffer is a hex digit
; and convert it to binary if it is. Carry set if not.

hexin:    mov       al,[si]

; Check if AL has a hex digit and convert it to binary if it
; is. Carry set if not.

hexchk:   sub       al,'0'              ; Kill ASCII numeric bias
          jc        ret2
          cmp       al,10
          cmc
          jnc       ret2                ; OK if 0-9
          and       al,5fh
          sub       al,7                ; Kill A-F bias
          cmp       al,10
          jc        ret2
          cmp       al,16
          cmc
ret2:     ret

; Process one parameter when a list of bytes is
; required. Carry set if parameter bad. Called by LIST.

listitem: call      scanp               ; Scan to parameter
          call      hexin               ; Is it in hex?
          jc        stringchk           ; If not, could be a string
          mov       cx,2                ; Only 2 hex digits for bytes
          call      gethex              ; Get the byte value
          mov       [bx],dl             ; Add to list
          inc       bx
gret:     clc                           ; Parameter was OK
          ret
stringchk:
          mov       al,[si]             ; Get first character of param
          cmp       al,"'"              ; String?
          jz        string
          cmp       al,'"'              ; Either quote is all right
          jz        string
          stc                           ; Not string, not hex - bad
          ret
string:   mov       ah,al               ; Save for closing quote
          inc       si
strnglp:  lodsb                         ; Next char of string
          cmp       al,13               ; Check for end of line
          jz        perr                ; Must find a close quote
          cmp       al,ah               ; Check for close quote
          jnz       stostrg             ; Add new character to list
          cmp       ah,[si]             ; Two quotes in a row?
          jnz       gret                ; If not, we're done
          inc       si                  ; Yes - skip second one
stostrg:  mov       [bx],al             ; Put new char in list
          inc       bx
          jmp short strnglp             ; Get more characters

; Get a byte list for ENTER, FILL or SEARCH. Accepts any number
; of 2-digit hex values or character strings in either single
; (') or double (") quotes.

list:     mov       bx,offset dg:bytebuf
listlp:   call      listitem            ; Process a parameter
          jnc       listlp              ; If OK, try for more
          sub       bx,offset dg:bytebuf
          jz        perror              ; List must not be empty

; Make sure there is nothing more on the line except for
; blanks and carriage return. If there is, it is an
; unrecognized parameter and an error.

geteol:   call      scanb               ; Skip blanks
          jnz       jump_error          ; Better be a RETURN
ret3:     ret

jump_error:
          jmp       word ptr [error_handler]

; Command error. SI has been incremented beyond the
; command letter so it must decremented for the
; error pointer to work.

perr:     dec       si

; Syntax error. SI points to character in the input buffer
; which caused error. By subtracting from start of buffer,
; we will know how far to tab over to appear directly below
; it on the terminal. Then print "^ Error".

perror:   sub       si,offset dg:(bytebuf-1)
          mov       cx,si               ; Parameter for TAB in CX
          call      tab                 ; Directly below bad char
          mov       si,offset dg:synerr ; Error message

; Print error message and abort to command level

print:    call      printmes
          jmp       command

; Gets an address in Segment:Displacement format. Segment may be omitted
; and a default (kept in BP) will be used, or it may be a segment
; register (DS, ES, SS, CS). Returns with segment in AX, OFFSET in DX.

address:  call      get_address
          jnc       adrerr
          jmp       jump_error
adrerr:   stc
          ret
get_address:
          call      scanp
          mov       al,[si+1]
          cmp       al,"S"
          jz        segreg
          mov       cx,4
          call      gethx
          jc        adrerr
          mov       ax,bp               ; Get default segment
          cmp       byte ptr [si],':'
          jnz       getret
          push      dx
getdisp:  inc       si                  ; Skip over ":"
          mov       cx,4
          call      gethx
          pop       ax
          jc        adrerr
getret:   clc
          ret
segreg:   mov       al,[si]
          mov       di,offset dg:seglet
          mov       cx,4
          repne scasb
          jnz       adrerr
          inc       si
          inc       si
          shl       cx,1
          mov       bx,cx
          cmp       byte ptr [si],":"
          jnz       adrerr
          push      [bx+dssave]
          jmp short getdisp

seglet:   db        "CSED"

; Short form of ENTER command. A list of values from the
; command line are put into memory without using normal
; ENTER mode.

getlist:  call      list                ; Get the bytes to enter
          pop       di                  ; Displacement within segment
          pop       es                  ; Segment to enter into
          mov       si,offset dg:bytebuf
          mov       cx,bx               ; Count of bytes
          rep movsb                     ; Enter that byte list
          ret

; Enter values into memory at a specified address. If the
; line contains nothing but the address we go into "enter
; mode", where the address and its current value are printed
; and the user may change it if desired. To change, type in
; new value in hex. Backspace works to correct errors. If
; an illegal hex digit or too many digits are typed, the
; bell is sounded but it is otherwise ignored. To go to the
; next byte (with or without change), hit space bar. To
; back   CLDto a previous address, type "-". On
; every 8-byte boundary a new line is started and the address
; is printed. To terminate command, type carriage return.
;   Alternatively, the list of bytes to be entered may be
; included on the original command line immediately following
; the address. This is in regular LIST format so any number
; of hex values or strings in quotes may be entered.

_enter:   mov       bp,[dssave]         ; Set default segment to DS
          call      address
          push      ax                  ; Save for later
          push      dx
          call      scanb               ; Any more parameters?
          jnz       getlist             ; If not end-of-line get list
          pop       di                  ; Displacement of ENTER
          pop       es                  ; Segment
getrow:   call      outdi               ; Print address of entry
          call      blank               ; Leave a space
          call      blank
getbyte:  mov       al,es:[di]          ; Get current value
          call      hex                 ; And display it
putdot:   mov       al,'.'
          call      _out                ; Prompt for new value
          mov       cx,2                ; Max of 2 digits in new value
          mov       dx,0                ; Intial new value
getdig:   call      _in                 ; Get digit from user
          mov       ah,al               ; Save
          call      hexchk              ; Hex digit?
          xchg      ah,al               ; Need original for echo
          jc        nohex               ; If not, try special command
          mov       dh,dl               ; Rotate new value
          mov       dl,ah               ; And include new digit
          loop      getdig              ; At most 2 digits

; We have two digits, so all we will accept now is a command.

dwait:
          call      _in                 ; Get command character
nohex:    cmp       al,8                ; Backspace
          jz        bs
          cmp       al,7fh              ; RUBOUT
          jz        rub
          cmp       al,'-'              ; Back to previous address
          jz        prev
          cmp       al,13               ; All done with command?
          jz        eol
          cmp       al,' '              ; Go to next address
          jz        next
          mov       al,8
          call      _out                ; Back over illegal character
          call      backup
          jcxz      dwait
          jmp short getdig
RUB:      mov       al,8
          call      _out
bs:       cmp       cl,2                ; CX=2 means nothing typed yet
          jz        putdot              ; Put back the dot we backed over
          inc       cl                  ; Accept one more character
          mov       dl,dh               ; Rotate out last digit
          mov       dh,ch               ; Zero this digit
          call      backup              ; Physical backspace
          jmp short getdig              ; Get more digits

; If new value has been entered, convert it to binary and
; put into memory. Always bump pointer to next location

store:    cmp       cl,2                ; CX=2 means nothing typed yet
          jz        nosto               ; So no new value to store

; Rotate DH left 4 bits to combine with DL and make a byte value

          push      cx
          mov       cl,4
          shl       dh,cl
          pop       cx
          or        dl,dh               ; Hex is now converted to binary
          mov       es:[di],dl          ; Store new value
nosto:    inc       di                  ; Prepare for next location
          ret
next:     call      store               ; Enter new value
          inc       cx                  ; Leave a space plus two for
          inc       cx                  ;  each digit not entered
          call      tab
          mov       ax,di               ; Next memory address
          and       al,7                ; Check for 8-byte boundary
          jnz       getbyte             ; Take 8 per line
newrow:   call      crlf                ; Terminate line
          jmp       getrow              ; Print address on new line
prev:     call      store               ; Enter the new value

; DI has been bumped to next byte. Drop it 2 to go to previous addr

          dec       di
          dec       di
          jmp short newrow              ; Terminate line after backing
eol:      call      store               ; Enter the new value
          jmp       crlf                ; CR/LF and terminate

; Console input of single character

_in:      mov       ah,1
          int       21h
          ret

code      ends
          end
