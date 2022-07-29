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
; THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT,TORT OR OTHERWISE, ARISING FROM
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
;
; MS-DOS is a Registered Trademark of Microsoft Corp.

false     equ       0
true      equ       not false

setcntc   equ       true                ; If this is FALSE, DEBUG will not set
                                        ; the Control C int vector

prompt    equ       '-'
fcb       equ       5Ch
exefcb    equ       fcb
buflen    equ       80                  ; Maximum length of line input buffer
bpmax     equ       10                  ; Maximum number of breakpoints
bplen     equ       5*bpmax             ; Length of breakpoint table
regtablen equ       14                  ; Number of registers
segdif    equ       0
bufsiz    equ       512

bxreg     equ       "B"+5800H           ; "BX"
bpreg     equ       "B"+5000H           ; "BP"
sireg     equ       "S"+4900H           ; "SI"
direg     equ       "D"+4900H           ; "DI"
comma     equ       2C00h
opbuflen  equ       35
