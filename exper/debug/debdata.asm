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

include   debequ.inc
include   dossym.inc

code      segment   public byte 'code'
          extrn     perror:near
code      ends

const     segment   public byte
const     ends

data      segment   public byte
data      ends

dg        group     code,const,data

data      segment   public byte

          public    ptyflag,xnxopt,xnxcmd,extptr,handle,transadd
          public    parserr,asmadd,disadd,discnt,asmsp,index,defdump,deflen
          public    regsave,segsave,offsave,temp,buffer,bytcnt,opcode,aword
          public    regmem,midfld,mode,nseg,opbuf,brkcnt,tcount,assemcnt
          public    assem1,assem2,assem3,assem4,assem5,assem6,bytebuf,bptab
          public    diflg,siflg,bxflg,bpflg,negflg,numflg,memflg,regflg
          public    movflg,tstflg,segflg,lownum,hinum,f8087,dirflg,dataend
          public    error_handler,zpcount
          public    int1save,int1saveseg,int3save,int3saveseg

error_handler:
          dw        perror

ptyflag:  db        0
xnxopt:   db        ?                   ; AL OPTION FOR DOS COMMAND
int1save: dw        ?
int1saveseg:
          dw        ?
int3save: dw        ?
int3saveseg:
          dw        ?
xnxcmd:   db        ?                   ; DOS COMMAND FOR OPEN_A_FILE TO PERFORM
extptr:   dw        ?                   ; POINTER TO FILE EXTENSION
handle:   dw        ?                   ; CURRENT HANDLE
transadd: dd        ?                   ; TRANSFER ADDRESS

parserr:  db        ?
asmadd:   db        4 dup (?)
disadd:   db        4 dup (?)
discnt:   dw        ?
asmsp:    dw        ?                   ; SP AT ENTRY TO ASM
index:    dw        ?
defdump:  db        4 dup (?)
deflen:   dw        ?
regsave:  dw        ?
segsave:  dw        ?
offsave:  dw        ?

; The following data areas are destroyed during hex file read

temp:     db        4 dup (?)
buffer    label     byte
bytcnt:   db        ?
opcode:   dw        ?
aword:    db        ?
regmem:   db        ?
midfld:   db        ?
mode:     db        ?
nseg:     dw        ?
opbuf:    db        opbuflen dup (?)
brkcnt:   dw        ?                   ; Number of breakpoints
tcount:   dw        ?                   ; Number of steps to trace
zpcount:  dw        ?                   ; Number of steps to zp trace
assemcnt: db        ?                   ; preserve order of assemcnt and assem1
assem1:   db        ?
assem2:   db        ?
assem3:   db        ?
assem4:   db        ?
assem5:   db        ?
assem6:   db        ?                   ; preserve order of assemx and bytebuf
bytebuf:  db        buflen  dup (?)     ; Table used by LIST
bptab:    db        bplen   dup (?)     ; Breakpoint table
diflg:    db        ?
siflg:    db        ?
bxflg:    db        ?
bpflg:    db        ?
negflg:   db        ?
numflg:   db        ?                   ; ZERO MEANS NO NUMBER SEEN
memflg:   db        ?
regflg:   db        ?
movflg:   db        ?
tstflg:   db        ?
segflg:   db        ?
lownum:   dw        ?
hinum:    dw        ?
f8087:    db        ?
dirflg:   db        ?
          db        buffer+bufsiz-$ dup (?)

dataend label   word

data    ends
        end
