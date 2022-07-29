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

          title     EDLIN for MSDOS 2.0

;-----------------------------------------------------------------------;
;               REVISION HISTORY:                                       ;
;                                                                       ;
;       V1.02                                                           ;
;                                                                       ;
;       V2.00   9/13/82  M.A. Ulloa                                     ;
;                  Modified to use Pathnames in command line file       ;
;               specification, modified REPLACE to use an empty         ;
;               string intead of the old replace string when this       ;
;               is missing, and search and replace now start from       ;
;               first line of buffer (like old version of EDLIN)        ;
;               instead than current+1 line. Also added the U and       ;
;               V commands that search (replace) starting from the      ;
;               current+1 line.                                         ;
;                                                                       ;
;               9/15/82  M.A. Ulloa                                     ;
;                  Added the quote character (^V).                      ;
;                                                                       ;
;               9/16/82  M.A. Ulloa                                     ;
;                  Corrected bug about use of quote char when going     ;
;               into default insert mode. Also corrected the problem    ;
;               with ^Z being the end of file marker. End of file is    ;
;               reached when an attempt to read returns less chars      ;
;               than requested.                                         ;
;                                                                       ;
;               9/17/82  M.A. Ulloa                                     ;
;                  Corrected bug about boundaries for Copy              ;
;                                                                       ;
;               10/4/82  Rev. 1         M.A. Ulloa                      ;
;                  The IBM version now does NOT have the U and V        ;
;               commands. The MSDOS version HAS the U and V commands.   ;
;                  Added the B switch, and modified the effect of       ;
;               the quote char.                                         ;
;                                                                       ;
;               10/7/82  Rev. 2         M.A. Ulloa                      ;
;                  Changed the S and R commands to start from the       ;
;               current line+1 (as U and V did). Took away U and V in   ;
;               all versions.                                           ;
;                                                                       ;
;               10/13/82 Rev. 3         M.A. Ulloa                      ;
;                  Now if parameter1 < 1 then parameter1 = 1            ;
;                                                                       ;
;               10/15/82 Rev. 4         M.A. Ulloa                      ;
;                  Param4 if specified must be an absolute number that  ;
;               reprecents the count.                                   ;
;                                                                       ;
;               10/18/82 Rev. 5         M.A. Ulloa                      ;
;                  Fixed problem with trying to edit files with the     ;
;               same name as directories. Also, if the end of file is   ;
;               reached it checks that a LF is the last character,      ;
;               otherwise it inserts a CRLF pair at the end.            ;
;                                                                       ;
;               10/20/82 Rev. 6         M.A.Ulloa                       ;
;                  Changed the text of some error messages for IBM and  ;
;               rewrite PAGE.                                           ;
;                                                                       ;
;               10/25/82 Rev. 7         M.A.Ulloa                       ;
;                  Made all messages as in the IBM vers.                ;
;                                                                       ;
;               10/28/82 Rev. 8         M.A.Ulloa                       ;
;                  Corrected problem with parsing for options.          ;
;                                                                       ;
;                        Rev. 9         Aaron Reynolds                  ;
;                  Made error messages external.                        ;
;                                                                       ;
;               12/08/82 Rev. 10        M.A. Ulloa                      ;
;                  Corrected problem arising with having to restore     ;
;               the old directory in case of a file name error.         ;
;                                                                       ;
;               12/17/82 Rev. 11        M.A. Ulloa                      ;
;                  Added the ROPROT equate for R/O file protection.     ;
;               It causes only certain operations (L,P,S,W,A, and Q)    ;
;               to be allowed on read only files.                       ;
;                                                                       ;
;               12/29/82 Rev. 12        M.A. Ulloa                      :
;                  Added the creation error message.                    ;
;                                                                       ;
;               4/14/83  Rev. 13        N.Panners                       ;
;                  Fixed bug in Merge which lost char if not ^Z.        ;
;                  Fixed bug in Copy to correctly check                 ;
;                  for full buffers.                                    ;
;                                                                       ;
;                                                                       ;
;               7/23/83 Rev. 14         N.Panners                       ;
;                   Split EDLIN into two seperate modules to            ;
;                   allow assembly of sources on an IBM PC              ;
;                   EDLIN and EDLPROC                                   ;
;                                                                       ;
;-----------------------------------------------------------------------;

; DOSLITE Revision History:
;
;   7/10/2022  usotsuki
;              Adapted to work with WASM.
;              Added /? switch for command-line help.
;              Added ? command for online help.
;                (The text taken from MS-DOS 5.  Must define HELP.)
;              Changed commands relying on screen dimensions to get line count
;                from the EGA/VGA if applicable, then fall back to 25 (as with
;                our version of MORE).
;              Removed cruft derived from excised features of MS-DOS 2.x.
;              Tidy up the code a bit.
;   7/11/2002  usotsuki
;              Add some sanity checking at startup.
;              Replace some baroque code with smaller equivalent not using
;                an undocumented MS-DOS function call.

false     equ       0
true      equ       not false

fcb       equ       5Ch

include   dossym.inc

prompt    equ       '*'
stksiz    equ       80h

code      segment   public
code      ends

const     segment   public word
const     ends

data      segment   public word
data      ends

dg        group     code,const,data

const     segment   public word

          extrn     baddrv:byte,ndname:byte,edos1:byte,opt_err:byte
          extrn     nobak:byte,badcom:byte,newfil:byte,dest:byte,mrgerr:byte
          extrn     nodir:byte,dskful:byte,memful:byte,filenm:byte
          extrn     nosuch:byte,toolng:byte,eof:byte,ro_err:byte,bcreat:byte
ifdef help
          extrn     helpmsg:byte,olhelp:byte
endif

          public    txt1,txt2,fudge,userdir,hardch

bak:      db        "BAK"

roflag:   db        0                   ; =1 if file is r/o
fourth:   db        0                   ; fourth parameter flag
loadmod:  db        0                   ; Load mode flag, 0 = ^Z marks the
                                        ; end of a file, 1 = viceversa.
hardch:   dd        ?
the_root: db        0                   ; root directory flag
fudge:    db        0                   ; directory changed flag
usrdrv:   db        0
scrnrows: dw        25
dirchar:  db        "\",0               ; Maintained for comparisons.
userdir:  db        "\",0               ; Needs \ at the beginning because
          db        (dirstrlen) dup(0)  ; MS-DOS "getcwd" call does not add it

fnambuf:  db        128 dup(0)
;-----------------------------------------------------------------------;

txt1:     db        0,80h dup (?)
txt2:     db        0,80h dup (?)
delflg:   db        0

const     ends

data      segment public word

          public    qflg,fcb2,oldlen,param1,param2,olddat,srchflg
          public    comline,newlen,srchmod,current,lstfnd,numpos
          public    lstnum,srchcnt,pointer,start,endtxt,usrdrv

;-----------------------------------------------------------------------;
;    Be careful when adding parameters, they have to follow the
; order in which they appear here. (this is a table, ergo it
; is indexed thru a pointer, and random additions will cause the
; wrong item to be accessed). Also param4 is known to be the
; count parameter, and known to be the fourth entry in the table
; so it receives special treatment. (See GETNUM)

param1:   dw        1 dup (?)
param2:   dw        1 dup (?)
param3:   dw        1 dup (?)
param4:   dw        1 dup (?)

;-----------------------------------------------------------------------;

ptr_1:    dw        1 dup (?)
ptr_2:    dw        1 dup (?)
ptr_3:    dw        1 dup (?)
copysiz:  dw        1 dup (?)
oldlen:   dw        1 dup (?)
newlen:   dw        1 dup (?)
lstfnd:   dw        1 dup (?)
lstnum:   dw        1 dup (?)
numpos:   dw        1 dup (?)
srchcnt:  dw        1 dup (?)
current:  dw        1 dup (?)
pointer:  dw        1 dup (?)
one4th:   dw        1 dup (?)
three4th: dw        1 dup (?)
last:     dw        1 dup (?)
endtxt:   dw        1 dup (?)
comline:  dw        1 dup (?)
lastlin:  dw        1 dup (?)
combuf:   db        82h dup (?)
editbuf:  db        258 dup (?)
eol:      db        1 dup (?)
fcb2:     db        37 dup (?)
fcb3:     db        37 dup (?)
fake_fcb: db        37 dup (?)          ; fake for size figuring
qflg:     db        1 dup (?)
haveof:   db        1 dup (?)
ending:   db        1 dup (?)
srchflg:  db        1 dup (?)
amnt_req: dw        1 dup (?)           ; amount of bytes requested to read
olddat:   db        1 dup (?)           ; Used in replace and search,
                                        ; replace by old data flag (1=yes)
srchmod:  db        1 dup (?)           ; Search mode: 1=from current+1 to
                                        ; end of buffer, 0=from beg. of
                                        ; buffer to the end (old way).
movflg:   db        1 dup (?)
          db        stksiz dup (?)

stack     label     byte
start     label     word
data      ends

code      segment   public

assume    cs:dg,ds:dg,ss:dg,es:dg

          extrn     quit:near,query:near,fndfirst:near,fndnext:near
          extrn     unquote:near,lf:near,crlf:near,_out:near
          extrn     rest_dir:near,kill_bl:near,int_24:near
          extrn     findlin:near,shownum:near,scanln:near

          public    chkrange

          org       100h

edlin:    jmp       simped

;edl_pad: db        0E00h dup (?)

noname:   mov       dx,offset dg:ndname
errj:     jmp       xerror

simped:   mov       byte ptr [ending],0
          mov       sp,offset dg:stack

;----- Check Version Number --------------------------------------------;
          push      ax
          mov       ah,Get_Version
          int       21h
          cmp       al,2
          jae       vers_ok             ; version >= 2, enter editor
          mov       dx,offset dg:edos1
          jmp       short errj
;-----------------------------------------------------------------------;

vers_ok:

ifdef     HELP
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

; You will want to disable this code if running on a non-compatible.

          mov       ah, 12h             ; do we have an EGA?
          mov       bx, 0FF10h
          mov       cx, 0FFFFh
          int       10h
          cmp       cx, 0FFFFh          ; unchanged = MDA or CGA
          je        noega               ;   = always 25 lines
          mov       ax, 40h             ; otherwise, lines-1 is stored at
          push      es                  ;   0040:0084
          mov       es, ax
          mov       al, [es:84h]
          pop       es
          inc       al
          xor       ah, ah
          mov       [scrnrows], ax
noega:

;----- Process Pathnames -----------------------------------------------;

          mov       si,81h              ; point to command line
          call      kill_bl
          cmp       al,13               ; A carriage return?
          je        noname              ; yes, file name missing
          mov       di,offset dg:fnambuf
          xor       cx,cx               ; zero pathname length

nxtchr:   stosb                         ; put pathname in buffer
          inc       cx
          lodsb
          cmp       al,' '
          je        xx1
          cmp       al,13               ; a CR ?
          je        gotname
          cmp       al,'/'              ; an option character?
          je        isopt
          jmp short nxtchr
xx1:      dec       si
          call      kill_bl
          cmp       al,'/'
          jne       gotname
isopt:    lodsb                         ; get the option
          cmp       al,'B'
          je        b_opt
          cmp       al,'b'
          je        b_opt
          mov       dx,offset dg:opt_err
          jmp       xerror              ; bad option specified
b_opt:    mov       [loadmod],1
gotname:  mov       byte ptr [di],0     ; nul terminate the pathname (uso: stripped dg:)

;----- Check that file is not R/O --------------------------------------;

          push      cx                  ; save character count
          mov       dx,offset dg:fnambuf
          mov       al,0                ; get attributes
          mov       ah,chmod
          int       21h
          jc        attr_ok
          test      cl, 10h             ; uso: is this a directory?
          jz        notdir              ;      nope, keep going.
          mov       dx, offset dg:baddrv
          jmp       xerror              ;      yes, die screaming.
notdir:   and       cl,00000001b        ; mask all but: r/o
          jz        attr_ok             ; if all = 0 then file ok to edit,
          mov       dg:[roflag],01h     ; otherwise: Error (GONG!!!)
attr_ok:  pop       cx                  ; restore character count

;----- Scan for directory ----------------------------------------------;
          dec       di                  ; adjust to the end of the pathname
          mov       al,'\'              ; get directory separator character
          std                           ; scan backwards
          repnz scasb                   ; (cx has the pathname length)
          cld                           ; reset direction, just in case
          jz        sj1
          jmp       same_dir            ; no dir separator char. found, the
                                        ; file is in the current directory
                                        ; of the corresponding drive. Ergo,
                                        ; the FCB contains the data already.
sj1:      jcxz      sj2                 ; no more chars left, refers to root
          cmp       byte ptr [di],':'   ; is the previous char a disk def?
          jne       not_root
sj2:      mov       dg:[the_root],01h   ; file is in the root
not_root: inc       di                  ; point to dir separator char.
          mov       al,0
          stosb                         ; nul terminate directory name
          pop       ax
          push      di                  ; save pointer to file name
          mov       dg:[fudge],01h      ; remember that the current directory
                                        ; has been changed.

;----- Save current directory for exit ---------------------------------;

; uso: The part from "samedrv" to the next heading originally used an
;      undocumented MS-DOS call, "getdpb".  This function did indeed return
;      the current directory, but in a baroque way that needed a lot of extra
;      code to extract it from an internal struct.  Since MS-DOS offered a
;      call to just ask for the damned thing, we can use that instead of all
;      the extra rigmarole, and save 32 bytes in the process.

          mov       ah,get_default_drive
          int       21h
          mov       dg:[usrdrv],al
          mov       dl,byte ptr ds:[fcb]
          or        dl,dl               ; default disk?
          jz        samedrv
          dec       dl                  ; adjust to real drive (a=0,b=1,...)
          mov       ah,set_default_drive
          int       21h
          cmp       al,-1               ; error?
          jne       samedrv
          mov       dx,offset dg:baddrv
          jmp       xerror
samedrv:  mov       ah, chdir
          mov       si, offset dg:userdir+1
          int       21h

;----- Change directories ----------------------------------------------;

          cmp       [the_root],01h
          mov       dx,offset dg:[dirchar]
          je        sj3
          mov       dx,offset dg:[fnambuf]
sj3:      mov       ah,chdir
          int       21h
          mov       dx,offset dg:baddrv
          jnc       noerrs
          jmp       xerror
noerrs:

;----- Set Up int 24 intercept -----------------------------------------;

          mov       ax,(get_interrupt_vector shl 8) or 24h
          int       21h
          mov       word ptr [hardch],bx
          mov       word ptr [hardch+2],es
          mov       ax,(set_interrupt_vector shl 8) or 24h
          mov       dx,offset dg:int_24
          int       21h
          push      cs
          pop       es

;----- Parse filename to FCB -------------------------------------------;

          pop       si
          mov       di,fcb
          mov       ax,(parse_file_descriptor shl 8) or 1
          int       21h
          push      ax

;-----------------------------------------------------------------------;

same_dir: pop       ax
          or        al,al
          mov       dx,offset dg:baddrv
          jz        sj4
          jmp       xerror
sj4:      cmp       byte ptr ds:[fcb+1],' '
          jnz       sj5
          jmp       noname
sj5:      mov       si,offset dg:bak    ; File must not have .BAK extension
          mov       di,fcb+9
          mov       cx,3
          repe cmpsb
          jz        notbak
          mov       ah,fcb_open         ; Open input file
          mov       dx,fcb
          int       21h
          mov       [haveof],al
          or        al,al
          jz        havfil

;----- Check that file is not a directory ------------

          mov       ah,fcb_create
          mov       dx,fcb
          int       21h
          or        al,al
          jz        sj50                ; no error found
          mov       dx,offset dg:bcreat ; creation error
          jmp       xerror
sj50:     mov       ah,fcb_close        ; no error, close the file
          mov       dx,fcb
          int       21h
          mov       ah,fcb_delete       ; delete the file
          mov       dx,fcb
          int       21h

;-----------------------------------------------------

          mov       dx,offset dg:newfil
          mov       ah,std_con_string_output
          int       21h
havfil:   mov       si,fcb
          mov       di,offset dg:fcb2
          mov       cx,9
          rep movsb
          mov       al,'$'
          stosb
          stosb
          stosb
makfil:   mov       dx,offset dg:fcb2   ; Create .$$$ file to make sure directory has room
          mov       ah,fcb_create
          int       21h
          or        al,al
          jz        setup
          cmp       byte ptr [delflg],0
          jnz       noroom
          call      delbak
          jmp       makfil
noroom:   mov       dx,offset dg:nodir
          jmp       xerror
notbak:   mov       dx,offset dg:nobak
          jmp       xerror
setup:    xor       ax,ax               ; Set RR field to zero
          mov       word ptr ds:[fcb+fcb_rr],ax         
          mov       word ptr ds:[fcb+fcb_rr+2],ax
          mov       word ptr [fcb2+fcb_rr],ax
          mov       word ptr [fcb2+fcb_rr+2],ax
          inc       ax                  ; Set record length to 1
          mov       word ptr ds:[fcb+fcb_recsiz],ax
          mov       word ptr [fcb2+fcb_recsiz],ax
          mov       dx,offset dg:start
          mov       di,dx
          mov       ah,set_dma
          int       21h
          mov       cx,ds:[6]
          dec       cx
          mov       [last],cx
          test      byte ptr [haveof],-1
          jnz       savend
          sub       cx,offset dg:start  ; Available memory
          shr       cx,1                ; 1/2 of available memory
          mov       ax,cx
          shr       cx,1                ; 1/4 of available memory
          mov       [one4th],cx         ; Save amount of 1/4 full
          add       cx,ax               ; 3/4 of available memory
          mov       dx,cx
          add       dx,offset dg:start
          mov       [three4th],dx       ; Save pointer to 3/4 full
          mov       dx,fcb              ; Read in input file
          mov       ah,fcb_random_read_block
          mov       [amnt_req],cx       ; save amount of chars requested
          int       21h
          call      scaneof
          add       di,cx               ; Point to last byte
savend:   cld
          mov       byte ptr [di],1Ah
          mov       [endtxt],di
          mov       byte ptr [combuf],128
          mov       byte ptr [editbuf],255
          mov       byte ptr [eol],10
          mov       [pointer],offset dg:start
          mov       [current],1
          mov       [param1],1
          test      byte ptr [haveof],-1
          jnz       command
          call      append
command:  mov       sp, offset dg:stack
          mov       ax,(set_interrupt_vector shl 8) or 23h
          mov       dx,offset dg:abortcom
          int       21h
          mov       al,prompt
          call      _out
          mov       dx,offset dg:combuf
          mov       ah,std_con_string_input
          int       21h
          mov       [comline],offset dg:combuf + 2
          mov       al,10
          call      _out
parse:    mov       [param2],0
          mov       [param3],0
          mov       [param4],0
          mov       [fourth],0          ; reset the fourth parameter flag
          mov       byte ptr [qflg],0
          mov       si,[comline]
          mov       bp,offset dg:param1
          xor       di,di
chklp:    call      getnum
          mov       [bp+di],dx
          inc       di
          inc       di
          call      skip1
ifdef     HELP
          cmp       al,'?'
          je        dohelp
endif
          cmp       al,','
          jnz       chknxt
          inc       si
chknxt:   dec       si
          cmp       di,8
          jb        chklp
          call      skip
          cmp       al,'?'
          jnz       dispatch
          mov       [qflg],al
          call      skip
dispatch: cmp       al,5Fh
          jbe       upcase
          and       al,5Fh
upcase:   mov       di,offset dg:comtab
          mov       cx,numcom
          repne scasb
          jnz       comerr
          mov       bx,cx
          mov       ax,[param2]
          or        ax,ax
          jz        parmok
          cmp       ax,[param1]
          jb        comerr              ; Param. 2 must be >= param 1
parmok:   mov       [comline],si
          cmp       [roflag],01         ; file r/o?
          jne       paramok2
          cmp       byte ptr [bx+rotable],01
          je        paramok2
          mov       dx,offset dg:ro_err
          jmp       short comerr1
paramok2: shl       bx,1
          call      [bx+table]
comover:  mov       si,[comline]
          call      skip
          cmp       al,0Dh
          jz        commandj
          cmp       al,1Ah
          jz        delim
          cmp       al,';'
          jnz       nodelim
delim:    inc       si
nodelim:  dec       si
          mov       [comline],si
          jmp       parse
commandj: jmp       command
skip:     lodsb
skip1:    cmp       al,' '
          jz        skip
ret1:     ret
chkrange: cmp       [param2],0
          jz        ret1
          cmp       bx,[param2]
          jbe       ret1
comerr:   mov       dx,offset dg:badcom
comerr1:  mov       ah,std_con_string_output
          int       21h
          jmp       command
getnum:   call      skip
          cmp       di,6                ; Is this the fourth parameter?
          jne       sk1
          mov       [fourth],1          ; yes, set the flag
sk1:      cmp       al,'.'
          jz        curlin
          cmp       al,'#'
          jz        maxlin
          cmp       al,'+'
          jz        forlin
          cmp       al,'-'
          jz        backlin
          mov       dx,0
          mov       cl,0                ; Flag no parameter seen yet
numlp:    cmp       al,'0'
          jb        numchk
          cmp       al,'9'
          ja        numchk
          cmp       dx,6553             ; Max line/10
          jae       comerr              ; Ten times this is too big
          mov       cl,1                ; Parameter digit has been found
          sub       al,'0'
          mov       bx,dx
          shl       dx,1
          shl       dx,1
          add       dx,bx
          shl       dx,1
          cbw
          add       dx,ax
          lodsb
          jmp       short numlp
numchk:   cmp       cl,0
          jz        ret1
          or        dx,dx
          jz        comerr              ; Don't allow zero as a parameter
          ret
curlin:   cmp       [fourth],1          ; the fourth parameter?
          je        comerra             ; yes, an error
          mov       dx,[current]
          lodsb
          ret
maxlin:   cmp       [fourth],1          ; the fourth parameter?
          je        comerra             ; yes, an error
          mov       dx,-2
          lodsb
          ret
forlin:   cmp       [fourth],1          ; the fourth parameter?
          je        comerra             ; yes, an error
          call      getnum
          add       dx,[current]
          ret
backlin:  cmp       [fourth],1          ; the fourth parameter?
          je        comerra             ; yes, an error
          call      getnum
          mov       bx,[current]
          sub       bx,dx
          jns       sk2                 ; if below beg of buffer then default
          mov       bx,1                ; to the beg of buffer (line1)
sk2:      mov       dx,bx
          ret
comerra:  jmp       comerr
ifdef     HELP
dohelp:   mov       dx, offset dg:olhelp
          mov       ah, std_con_string_output
          int       21h
          jmp       command
endif

comtab:   db        "QTCMWASRDLPIE;",13
numcom    equ       $-comtab

;-----------------------------------------------------------------------;
;      Careful changing the order of the next two tables. They are
;      linked and chnges should be be to both.

table:    dw        nocom               ; No command--edit line
          dw        nocom
          dw        ended
          dw        insert
          dw        _page
          dw        list
          dw        delete
          dw        repldown            ; replace from current+1 line
          dw        srchdown            ; search from current+1 line
          dw        append
          dw        ewrite
          dw        move
          dw        copy
          dw        merge
          dw        quit1

;-----------------------------------------------------------------------;
;       If = 1 then the command can be executed with a file that
;      is r/o. If = 0 the command can not be executed, and error.

rotable:  db        0                   ; nocom
          db        0                   ; nocom
          db        0                   ; ended
          db        0                   ; insert
          db        1                   ; page
          db        1                   ; list
          db        0                   ; delete
          db        0                   ; repldown
          db        1                   ; srchdown
          db        1                   ; append
          db        1                   ; ewrite
          db        0                   ; move
          db        0                   ; copy
          db        0                   ; merge
          db        1                   ; quit

;-----------------------------------------------------------------------;

quit1:    cmp       [roflag],01         ; are we in r/o mode?
          jne       q3                  ; no query....
          mov       dx,offset dg:fcb2   ; yes, quit without query.
          mov       ah,fcb_close
          int       21h
          mov       ah,fcb_delete
          int       21h
          call      rest_dir            ; restore directory if needed
          int       20h
q3:       call      quit
scaneof:  cmp       [loadmod],0
          je        sj52

;----- Load till physical end of file
          
          cmp       cx,word ptr[amnt_req]
          jb        sj51
          xor       al,al
          inc       al                  ; reset zero flag
          ret
sj51:     jcxz      sj51b
          push      di                  ; get rid of any ^Z at end of file
          add       di,cx
          dec       di                  ; points to last char
          cmp       byte ptr [di],1Ah
          pop       di
          jne       sj51b
          dec       cx
sj51b:    xor       al,al               ; set zero flag
          call      chkend              ; check that we have CRLF pair at end
          ret

;----- Load till first ^Z is found

sj52:     push      di
          push      cx
          mov       al,1Ah
          or        cx,cx
          jz        notfound            ; skip with zero flag set
          repne scasb                   ; Scan for end of file mark
          jnz       notfound
          lahf                          ; Save flags momentarily
          inc       cx                  ; include the ^Z
          sahf                          ; Restore flags
notfound: mov       di,cx               ; not found at the end
          pop       cx
          lahf                          ; Save flags momentarily
          sub       cx,di               ; Reduce byte count if EOF found
          sahf                          ; Restore flags
          pop       di
          call      chkend              ; check that we have CRLF pair at end
ret2:     ret

;-----------------------------------------------------------------------
;       If the end of file was found, then check that the last character
; in the file is a LF. If not put a CRLF pair in.

chkend:   jnz       not_end             ; end was not reached
          pushf                         ; save return flag
          push      di                  ; save pointer to buffer
          add       di,cx               ; points to one past end on text
          dec       di                  ; points to last character
          cmp       di,offset dg:start
          je        chk_no
          cmp       byte ptr[di],0Ah    ; is a LF the last character?
          je        chk_done            ; yes, exit
chk_no:   mov       byte ptr[di+1],0Dh  ; no, put a CR
          inc       cx                  ; one more char in text
          mov       byte ptr[di+2],0Ah  ; put a LF
          inc       cx                  ; another character at the end
chk_done: pop       di
          popf
not_end:  ret
nomorej:  jmp       nomore
append:   test      byte ptr [haveof],-1
          jnz       nomorej
          mov       dx,[endtxt]
          cmp       [param1],0          ; See if parameter is missing
          jnz       parmapp
          cmp       dx,[three4th]       ; See if already 3/4ths full
          jae       ret2                ; If so, then done already
parmapp:  mov       di,dx
          mov       ah,set_dma
          int       21h
          mov       cx,[last]
          sub       cx,dx               ; Amount of memory available
          jnz       sj53
          jmp       memerr
sj53:     mov       dx,fcb
          mov       [amnt_req],cx       ; save ammount of chars requested
          mov       ah,fcb_random_read_block
          int       21h                 ; Fill memory with file data
          mov       [haveof],al
          push      cx                  ; Save actual byte count
          call      scaneof
          jnz       notend
          mov       byte ptr [haveof],1 ; Set flag if 1AH found in file
notend:   xor       dx,dx
          mov       bx,[param1]
          or        bx,bx
          jnz       countln
          mov       ax,di
          add       ax,cx               ; First byte after loaded text
          cmp       ax,[three4th]       ; See if we made 3/4 full
          jbe       countln
          mov       di,[three4th]
          mov       cx,ax
          sub       cx,di               ; Length remaining over 3/4
          mov       bx,1                ; Look for one more line
countln:  call      scanln              ; Look for BX lines
          cmp       [di-1],al           ; Check for full line
          jz        fulln
          std
          dec       di
          mov       cx,[last]
          repne scasb                   ; Scan backwards for last line
          inc       di
          inc       di
          dec       dx
          cld
fulln:    pop      cx                   ; Actual amount read
          mov      word ptr [di],1Ah    ; Place EOF after last line
          sub      cx,di
          xchg     di,[endtxt]
          add      di,cx                ; Amount of file read but not used
          sub      word ptr ds:[fcb+fcb_rr],di
          sbb      word ptr ds:[fcb+fcb_rr+2],0
          cmp      bx,dx
          jnz      eofchk
          mov      byte ptr [haveof],0
          ret
nomore:   mov       dx,offset dg:eof
          mov       ah,std_con_string_output
          int       21h
ret3:     ret
eofchk:   test      byte ptr [haveof],-1
          jnz       nomore
          test      byte ptr [ending],-1
          jnz       ret3                ; Suppress memory error during End
          jmp       memerr
ewrite:   mov       bx,[param1]
          or        bx,bx
          jnz       wrt
          mov       cx,[one4th]
          mov       di,[endtxt]
          sub       di,cx               ; Write everything in front of here
          jbe       ret3
          cmp       di,offset dg:start  ; See if there's anything to write
          jbe       ret3
          xor       dx,dx
          mov       bx,1                ; Look for one more line
          call      scanln
          jmp short wrtadd
wrt:      inc       bx
          call      findlin
wrtadd:   cmp       byte ptr [delflg],0
          jnz       wrtadd1
          push      di
          call      delbak              ; Want to delete the .BAK file
                                        ; as soon as the first write occurs
          pop       di
wrtadd1:  mov       cx,di
          mov       dx,offset dg:start
          sub       cx,dx               ; Amount to write
          jz        ret3
          mov       ah,set_dma
          int       21h
          mov       dx,offset dg:fcb2
          mov       ah,fcb_random_write_block
          int       21h
          or        al,al
          jnz       wrterr
          mov       si,di
          mov       di,offset dg:start
          mov       [pointer],di
          mov       cx,[endtxt]
          sub       cx,si
          inc       cx                  ; Amount of text remaining
          rep       movsb
          dec       di                  ; Point to EOF
          mov       [endtxt],di
          mov       [current],1
          ret
wrterr:   mov       ah,fcb_close
          int       21h
          mov       dx,offset dg:dskful
xerror:   mov       ah,std_con_string_output
          int       21h
;-----------------------------------------------------------------------
          call      rest_dir                ;restore to the proper directory
;-----------------------------------------------------------------------
          int       20h
ret$5:    ret
_page:    xor       bx,bx               ; get last line in the buffer
          call      findlin
          mov       [lastlin],dx
          mov       bx,[param1]
          or        bx,bx               ; was it specified?
          jnz       frstok              ; yes, use it
          mov       bx,[current]
          cmp       bx,1                ; if current line =1 start from there
          je        frstok
          inc       bx                  ; start from current+1 line
frstok:   cmp       bx,[lastlin]        ; check that we are in the buffer
          ja        ret$5               ; if not just quit
infile:   mov       dx,[param2]
          or        dx,dx               ; was param2 specified?
          jnz       scndok              ; yes,....
          mov       dx,bx               ; no, take the end line to be the
          add       dx,[scrnrows]       ; start line + 23
          sub       dx, 3
scndok:   inc       dx
          cmp       dx,[lastlin]        ; check that we are in the buffer
          jbe       infile2
          mov       dx,[lastlin]        ; we are not, take the last line as end
infile2:  cmp       dx,bx               ; is param1 < param2 ?
          jbe       ret$5               ; yes, no backwards listing, quit
          push      dx                  ; save the end line
          push      bx                  ; save start line
          mov       bx,dx               ; set the current line
          dec       bx
          call      findlin
          mov       [pointer],di
          mov       [current],dx
          pop       bx                  ; restore start line
          call      findlin             ; get pointer to start line
          mov       si,di               ; save pointer
          pop       di                  ; get end line
          sub       di,bx               ; number of lines
          jmp short display
list:     mov       bx,[param1]
          or        bx,bx
          jnz       chkp2
          mov       bx,[current]
          sub       bx,11
          ja        chkp2
          mov       bx,1
chkp2:    call      findlin
          jnz       ret7
          mov       si,di
          mov       di,[param2]
          inc       di
          sub       di,bx
          ja        display
          mov       di,[scrnrows]
          dec       di
          dec       di
          jmp       short display
dispone:  mov       di,1

; Inputs:
;       BX = Line number
;       SI = Pointer to text buffer
;       DI = No. of lines
; Function:
;       Ouputs specified no. of line to terminal, each
;       with leading line number.
; Outputs:
;       BX = Last line output.
; All registers destroyed.

display:  mov     cx,[endtxt]
          sub     cx,si
          jz      ret7
          mov     bp,[current]
displn:   push    cx
          call    shownum
          pop     cx
outln:    lodsb
          cmp      al,' '
          jae      send
          cmp      al,10
          jz       send
          cmp      al,13
          jz       send
          cmp      al,9
          jz       send
          push     ax
          mov      al,'^'
          call     _out
          pop      ax
          or       al,40h
send:     call     _out
          cmp      al,10
          loopnz   outln
          jcxz     ret7
          inc      bx
          dec      di
          jnz      displn
          dec      bx
ret7:     ret
loadbuf:  mov       di,offset dg:editbuf+2
          mov       cx,255
          mov       dx,-1
loadlp:   lodsb
          stosb
          inc       dx
          cmp       al,13
          loopnz    loadlp
          mov       [editbuf+1],dl
          jz        ret7
trunclp:  lodsb
          inc       dx
          cmp       al,13
          jnz       trunclp
          dec       di
          stosb
          ret
notfndj:  jmp       notfnd
repldown: mov       byte ptr [srchmod],1   ;search from curr+1 line
          jmp       short sj6
replac:   mov       byte ptr [srchmod],0   ;search from beg of buffer
sj6:      mov       byte ptr [srchflg],0
          call      fndfirst
          jnz       notfndj
replp:    mov       si,[numpos]
          call      loadbuf             ; Count length of line
          sub       dx,[oldlen]
          mov       cx,[newlen]
          add       dx,cx               ; Length of new line
          cmp       dx,254
          ja        toolong
          mov       bx,[lstnum]
          push      dx
          call      shownum
          pop       dx
          mov       cx,[lstfnd]
          mov       si,[numpos]
          sub       cx,si               ; Get # of char on line before change
          dec       cx
          call      outcnt              ; Output first part of line
          push      si
          mov       si, offset dg:txt2+1
          mov       cx,[newlen]
          call      outcnt              ; Output change
          pop       si
          add       si,[oldlen]         ; Skip over old stuff in line
          mov       cx,dx               ; DX=no. of char left in line
          add       cx,2                ; Include CR/LF
          call      outcnt              ; Output last part of line
          call      query               ; Check if change OK
          jnz       repnxt
          call      putcurs
          mov       di,[lstfnd]
          dec       di
          mov       si, offset dg:txt2+1
          mov       dx,[oldlen]
          mov       cx,[newlen]
          dec       cx
          add       [lstfnd],cx         ; Bump pointer beyond new text
          inc       cx
          dec       dx
          sub       [srchcnt],dx        ; Old text will not be searched
          jae       someleft
          mov       [srchcnt],0
someleft: inc       dx
          call      replace
repnxt:   call      fndnext
          jnz       ret8
          jmp       replp
outcnt:   jcxz      ret8
outlp:    lodsb
          call      _out
          dec       dx
          loop      outlp
ret8:     ret
toolong:  mov       dx,offset dg:toolng
          jmp       short perr
srchdown: mov       byte ptr [srchmod],1
          jmp       short sj7
search:   mov       byte ptr [srchmod],0
sj7:      mov       byte ptr [srchflg],1
          call      fndfirst
          jnz       notfnd
srch:     mov       bx,[lstnum]
          mov       si,[numpos]
          call      dispone
          mov       di,[lstfnd]
          mov       cx,[srchcnt]
          mov       al,10
          repne scasb
          jnz       notfnd
          mov       [lstfnd],di
          mov       [numpos],di
          mov       [srchcnt],cx
          inc       [lstnum]
          call      query
          jz        putcurs
          call      fndnext
          jz        srch
notfnd:   mov       dx,offset dg:nosuch
perr:     mov       ah,std_con_string_output
          int       21h
          ret
putcurs:  mov       bx,[lstnum]
          dec       bx                  ; Current <= Last matched line
          call      findlin
          mov       [current],dx
          mov       [pointer],di
ret9:     ret
delete:   mov       bx,[param1]
          or        bx,bx
          jnz       delfin1
          mov       bx,[current]
          call      chkrange
delfin1:  call      findlin
          jnz       ret9
          push      bx
          push      di
          mov       bx,[param2]
          or        bx,bx
          jnz       delfin2
          mov       bx,dx
delfin2:  inc       bx
          call      findlin
          mov       dx,di
          pop       di
          sub       dx,di
          jbe       comerrj
          pop       [current]
          mov       [pointer],di
          xor       cx,cx
          jmp short replace
comerrj:  jmp       comerr
nocom:    dec       [comline]
          mov       bx,[param1]
          or        bx,bx
          jnz       havlin
          mov       bx,[current]
          inc       bx                  ; Default is current line plus one
          call      chkrange
havlin:   call      findlin
          mov       si,di
          mov       [current],dx
          mov       [pointer],si
          jz        sj12
          ret
sj12:     cmp       si,[endtxt]
          jz        ret12
          call      loadbuf
          mov       [oldlen],dx
          mov       si,[pointer]
          call      dispone
          call      shownum
          mov       ah,std_con_string_input
          mov       dx,offset dg:editbuf
          int       21h
          mov       al,10
          call      _out
          mov       cl,[editbuf+1]
          mov       ch,0
          jcxz      ret12
          mov       dx,[oldlen]
          mov       si,offset dg:editbuf+2
;-----------------------------------------------------------------------
          call      unquote             ; scan for quote chars if any
;-----------------------------------------------------------------------
          mov       di,[pointer]

; Inputs:
;       CX = Length of new text
;       DX = Length of original text
;       SI = Pointer to new text
;       DI = Pointer to old text in buffer
; Function:
;       New text replaces old text in buffer and buffer
;       size is adjusted. CX or DX may be zero.
; CX, SI, DI all destroyed. No other registers affected.

replace:  cmp       cx,dx
          jz        copyin
          push      si
          push      di
          push      cx
          mov       si,di
          add       si,dx
          add       di,cx
          mov       ax,[endtxt]
          sub       ax,dx
          add       ax,cx
          cmp       ax,[last]
          jae       memerr
          xchg      ax,[endtxt]
          mov       cx,ax
          sub       cx,si
          cmp       si,di
          ja        domov
          add       si,cx
          add       di,cx
          std
domov:    inc       cx
          rep movsb
          cld
          pop       cx
          pop       di
          pop       si
copyin:   rep movsb
ret12:    ret
memerr:   mov       dx,offset dg:memful
          mov       ah,std_con_string_output
          int       21h
          jmp       command
moverr:   mov       dx,offset dg:badcom
errorj:   jmp       comerr
move:     mov       byte ptr [movflg],1
          jmp       short blkmove
copy:     mov       byte ptr [movflg],0
blkmove:  mov       bx,[param3]         ; Third parameter must be specified
          or        bx,bx
          mov       dx,offset dg:dest
          jz        errorj
          mov       bx,[param1]         ; Get the first parameter
          or        bx,bx               ; Not specified?
          jnz       nxtarg
          mov       bx,[current]        ; Defaults to the current line
          call      chkrange
          mov       [param1],bx         ; Save it since current line may change
nxtarg:   call      findlin             ; Get a pointer to the line
          mov       [ptr_1],di          ; Save it
          mov       bx,[param2]         ; Get the second parameter
          or        bx,bx               ; Not specified?
          jnz       havargs
          mov       bx,[current]        ; Defaults to the current line
          mov       [param2],bx         ; Save it since current line may change
havargs:  mov       dx,[param3]         ; Parameters must not overlap
          cmp       dx,[param1]
          jbe       noerror
          cmp       dx,[param2]
          jbe       moverr
noerror:  inc       bx                  ; Get pointer to line Param2+1
          call      findlin
          mov       si,di
          mov       [ptr_2],si          ; Save it
          mov       cx,di
          mov       di,[ptr_1]          ; Restore pointer to line Param1
          sub       cx,di               ; Calculate number of bytes to copy
          mov       [copysiz],cx        ; Save in COPYSIZ
          push      cx                  ; And on the stack
          mov       ax,[param4]         ; Is count specified?
          or        ax,ax
          jz        memchk
          mul       [copysiz]
          or        dx,dx
          jz        cpsz_ok
          jmp       memerr
cpsz_ok:  mov       cx,ax
          mov       [copysiz],cx
memchk:   mov       ax,[endtxt]
          mov       di,[last]
          sub       di,ax
          cmp       di,cx
          jae       hav_room
          jmp       memerr
hav_room: mov       bx,[param3]
          push      bx
          call      findlin
          mov       [ptr_3],di
          mov       cx,[endtxt]
          sub       cx,di
          inc       cx
          mov       si,[endtxt]
          mov       di,si
          add       di,[copysiz]
          mov       [endtxt],di
          std
          rep movsb
          cld
          pop       bx
          cmp       bx,[param1]
          jb        getptr2
          mov       si,[ptr_1]
          jmp       short cpytxt
getptr2:  mov       si,[ptr_2]
cpytxt:   mov       bx,[param4]
          mov       di,[ptr_3]
          pop       cx
          mov       [copysiz],cx
cpytxt_1: rep movsb
          dec       bx
          cmp       bx,0
          jle       mov_chk
          mov       [param4],bx
          sub       si,[copysiz]
          mov       cx,[copysiz]
          jmp short cpytxt_1
mov_chk:  cmp       byte ptr[movflg],0
          jz        copydone
          mov       di,[ptr_1]
          mov       si,[ptr_2]
          mov       bx,[param3]
          cmp       bx,[param1]
          jae       del_text
          add       di,[copysiz]
          add       si,[copysiz]
del_text: mov       cx,[endtxt]
          sub       cx,si
          rep movsb
          mov       [endtxt],di
          mov       cx,[param2]
          sub       cx,[param1]
          mov       bx,[param3]
          sub       bx,cx
          jnc       movedone
copydone: mov       bx,[param3]
movedone: call      findlin
          mov       [pointer],di
          mov       [current],bx
          ret
movefile: mov       cx,[endtxt]         ; Get End-of-text marker
          mov       si,cx
          sub       cx,di               ; Calculate number of bytes to copy
          inc       cx
          mov       di,dx
          std
          rep movsb                     ; Copy CX bytes
          xchg      si,di
          cld
          inc       di
          mov       bp,si
setpts:   mov       [pointer],di        ; Current line is first free loc
          mov       [current],bx        ;   in the file
          mov       [endtxt],bp         ; End-of-text is last free loc before
          ret
namerr:   jmp       comerr1
merge:    mov       ax,(parse_file_descriptor shl 8) or 1
          mov       di,offset dg:fcb3
          int       21h
          or        al,al
          mov       dx,offset dg:baddrv
          jnz       namerr
          mov       [comline],si
          mov       dx,offset dg:fcb3
          mov       ah,fcb_open
          int       21h
          or        al,al
          mov       dx,offset dg:filenm
          jnz       namerr
          mov       ax,(set_interrupt_vector shl 8) or 23h
          mov       dx,offset dg:abrtmrg
          int       21h
          mov       bx,[param1]
          or        bx,bx
          jnz       mrg
          mov       bx,[current]
          call      chkrange
mrg:      call      findlin
          mov       bx,dx
          mov       dx,[last]
          call      movefile
          mov       dx,[pointer]        ; Set DMA address for reading in new file
          mov       ah,set_dma
          int       21h
          xor       ax,ax
          mov       word ptr ds:[fcb3+fcb_rr],ax
          mov       word ptr ds:[fcb3+fcb_rr+2],ax
          inc       ax
          mov       word ptr ds:[fcb3+fcb_recsiz],ax
          mov       dx,offset dg:fcb3
          mov       cx,[endtxt]
          sub       cx,[pointer]
          mov       ah,fcb_random_read_block
          int       21h
          cmp       al,1
          jz        filemrg
          mov       dx,offset dg:mrgerr
          mov       ah,std_con_string_output
          int       21h
          mov       cx,[pointer]
          jmp short restore
filemrg:  add       cx,[pointer]
          mov       si,cx
          dec       si
          lodsb
          cmp       al,1Ah
          jnz       restore
          dec       cx
restore:  mov       di,cx
          mov       si,[endtxt]
          inc       si
          mov       cx,[last]
          sub       cx,si
          rep movsb
          mov       [endtxt],di
          mov       byte ptr [di],1Ah
          mov       dx,offset dg:fcb3
          mov       ah,fcb_close
          int       21h
          mov       dx,offset dg:start
          mov       ah,set_dma
          int       21h
          ret
insert:   mov       ax,(set_interrupt_vector shl 8) or 23h
          mov       dx,offset dg:abortins
          int       21h
          mov       bx,[param1]
          or        bx,bx
          jnz       _ins
          mov       bx,[current]
          call      chkrange
_ins:     call      findlin
          mov       bx,dx
          mov       dx,[last]
          call      movefile
inlp:     call      setpts              ; Update the pointers into file
          call      shownum
          mov       dx,offset dg:editbuf
          mov       ah,std_con_string_input
          int       21h
          call      lf
          mov       si,2 + offset dg:editbuf
          cmp       byte ptr [si],1Ah
          jz        endins
;-----------------------------------------------------------------------
          call      unquote             ; scan for quote chars if any
;-----------------------------------------------------------------------
          mov       cl,[si-1]
          mov       ch,0
          mov       dx,di
          inc       cx
          add       dx,cx
          jc        memerrj1
          jz        memerrj1
          cmp       dx,bp
          jb        memok
memerrj1: call      end_ins
          jmp       memerr
memok:    rep movsb
          mov       al,10
          stosb
          inc       bx
          jmp short inlp
abrtmrg:  mov       dx,offset dg:start
          mov       ah,set_dma
          int       21h
abortins: mov       ax,cs               ; Restore segment registers
          mov       ds,ax
          mov       es,ax
          mov       ss,ax
          mov       sp,offset dg:stack
          sti
          call      crlf
          call      endins
          jmp       comover
endins:   call      end_ins
          ret
end_ins:  mov       bp,[endtxt]
          mov       di,[pointer]
          mov       si,bp
          inc       si
          mov       cx,[last]
          sub       cx,bp
          rep movsb
          dec       di
          mov       [endtxt],di
          mov       ax,(set_interrupt_vector shl 8) or 23h
          mov       dx,offset dg:abortcom
          int       21h
          ret
fillbuf:  mov       [param1],-1         ; Read in max. no of lines
          call      append
ended:    mov       byte ptr [ending],1 ; Suppress memory errors
          mov       bx,-1               ; Write max. no of lines
          call      wrt
          test      byte ptr [haveof],-1
          jz        fillbuf
          mov       dx,[endtxt]
          mov       ah,set_dma
          int       21h
          mov       cx,1
          mov       dx,offset dg:fcb2
          mov       ah,fcb_random_write_block
          int       21h                 ; Write end-of-file byte
          mov       ah,fcb_close        ; Close .$$$ file
          int       21h
          mov       si,fcb
          lea       di,[si+fcb_filsiz]
          mov       dx,si
          mov       cx,9
          rep movsb
          mov       si,offset dg:bak
          movsw
          movsb
          mov       ah,fcb_rename       ; Rename original file .BAK
          int       21h
          mov       si,fcb
          mov       di,offset dg:fcb2+fcb_filsiz
          mov       cx,6
          rep movsw
          mov       dx,offset dg:fcb2   ; Rename .$$$ file to original name
          int       21h
          call      rest_dir            ; restore directory if needed
          int       20h
abortcom: mov       ax,cs
          mov       ds,ax
          mov       es,ax
          mov       ss,ax
          mov       sp,offset dg:stack
          sti
          call      crlf
          jmp       command
delbak:   mov       byte ptr [delflg],1
          mov       di,9+offset dg:fcb2
          mov       si,offset dg:bak
          movsw
          movsb
          mov       ah,fcb_delete       ; Delete old backup file (.BAK)
          mov       dx,offset dg:fcb2
          int       21h
          mov       di,9+offset dg:fcb2
          mov       al,'$'
          stosb
          stosb
          stosb
          ret

code      ends
          end       edlin
