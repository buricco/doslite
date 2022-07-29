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

;-----------------------------------------------------------------------;
;                                                                       ;
;       Done for Vers 2.00 (rev 9) by Aaron Reynolds                    ;
;       Update for rev. 11 by M.A. Ulloa                                ;
;                                                                       ;
;-----------------------------------------------------------------------;

false     equ       0
true      equ       not false

include   dossym.inc

code      segment   public byte
code      ends

const     segment   public byte
const     ends

data      segment   public byte
          extrn     qflg:byte,fcb2:byte
data      ends

dg        group     code,const,data

code      segment   public byte

assume    cs:dg,ds:dg,ss:dg,es:dg

          public    quit,query
          extrn     rest_dir:near,crlf:near

quit:     mov       dx,offset dg:qmes
          mov       ah,std_con_string_output
          int       21h
          mov       ax,(std_con_input_flush shl 8) or std_con_input
          int       21h
          and       al,5Fh
          cmp       al,'Y'
          jz        nocrlf
          jmp       crlf
nocrlf:   mov       dx,offset dg:fcb2
          mov       ah,fcb_close
          int       21h
          mov       ah,fcb_delete
          int       21h
          call      rest_dir
          int       20h
query:    test      byte ptr [qflg],-1
          jz        ret9
          mov       dx,offset dg:ask
          mov       ah,std_con_string_output
          int       21h
          mov       ax,(std_con_input_flush shl 8) or std_con_input
          int       21h
          push      ax
          call      crlf
          pop       ax
          cmp       al,13               ; Carriage return means yes
          jz        ret9
          cmp       al,'Y'
          jz        ret9
          cmp       al,'y'
ret9:     ret

code      ends

const     segment   public byte

          public    baddrv,ndname,edos1,opt_err,nobak
          public    nodir,dskful,memful,filenm,badcom,newfil
          public    nosuch,toolng,eof,dest,mrgerr,ro_err,bcreat
ifdef   help
          public    helpmsg, olhelp
endif

baddrv:   db        "Invalid drive or file name",13,10,"$"
ndname:   db        "File name must be specified",13,10,"$"

edos1:    db        "Incorrect DOS version",13,10,"$"
opt_err:  db        "Invalid parameter",13,10,"$"
ro_err:   db        "File is READ-ONLY",13,10,"$"
bcreat:   db        "File creation error",13,10,"$"

nobak:    db        "Cannot edit .BAK file--rename file",13,10,"$"
nodir:    db        "No room in directory for file",13,10,"$"
dskful:   db        "Disk full. Edits lost.",13,10,"$"
memful:   db        13,10,"Insufficient memory",13,10,"$"
filenm:   db        "File not found",13,10,"$"
badcom:   db        "Entry error",13,10,"$"
newfil:   db        "New file",13,10,"$"
nosuch:   db        "Not found",13,10,"$"
ask:      db        "O.K.? $"
toolng:   db        "Line too long",13,10,"$"
eof:      db        "End of input file",13,10,"$"
qmes:     db        "Abort edit (Y/N)? $"
dest:     db        "Must specify destination line number",13,10,"$"
mrgerr:   db        "Not enough room to merge the entire file",13,10,"$"

ifdef   HELP
helpmsg:
          db        "Starts Edlin, a line-oriented text editor.", 13, 10, 13, 10
          db        "EDLIN [drive:][path]filename [/B]", 13, 10, 13, 10
          db        "  /B   Ignores end-of-file (CTRL+Z) characters.", 13, 10, "$"
olhelp:
          db        "Edit line                   line#", 13, 10
          db        "Append                      [#lines]A", 13, 10
          db        "Copy                        [startline],[endline],toline[,times]C", 13, 10
          db        "Delete                      [startline][,endline]D", 13, 10
          db        "End (save file)             E", 13, 10
          db        "Insert                      [line]I", 13, 10
          db        "List                        [startline][,endline]L", 13, 10
          db        "Move                        [startline],[endline],tolineM", 13, 10
          db        "Page                        [startline][,endline]P", 13, 10
          db        "Quit (throw away changes)   Q", 13, 10
          db        "Replace                     [startline][,endline][?]R[oldtext][CTRL+Znewtext]", 13, 10
          db        "Search                      [startline][,endline][?]Stext", 13, 10
          db        "Transfer                    [toline]T[drive:][path]filename", 13, 10
          db        "Write                       [#lines]W", 13, 10, "$"
endif

const   ends
        end
