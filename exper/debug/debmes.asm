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

false     equ       0
true      equ       not false

          include   dossym.inc

firstdrv  equ       'A'

code      segment   public byte 'code'
code      ends

const     segment   public byte
const     ends

data      segment   public byte
          extrn     ptyflag:byte
data      ends

dg        group     code,const,data

code      segment   public byte 'code'
assume    cs:dg,ds:dg,es:dg,ss:dg

          extrn     rprbuf:near,restart:near
          public    drverr, trapparity, releaseparity, nmiint, nmiintend
trapparity:
          ret
releaseparity:
          ret

nmiint:
nmiintend:

drverr:   mov       dx,offset dg:disk
          or        al,al
          jnz       savdrv
          mov       dx,offset dg:wrtpro
savdrv:   push      cs
          pop       ds
          push      cs
          pop       es
          add       byte ptr drvlet,firstdrv
          mov       si,offset dg:readm
          mov       di,offset dg:errtyp
          cmp       byte ptr rdflg,write
          jnz       movmes
          mov       si,offset dg:writm
movmes:   movsw
          movsw
          call      rprbuf
          mov       dx,offset dg:dskerr
          jmp       restart
codeend:

code      ends


const     segment   public byte

ifdef     help
          public    helpmsg,olhelp
endif
          public    badver,endmes,carret,nambad,notfnd,noroom
          public    nospace,drvlet
          public    accmes
          public    toobig,synerr,errmes,bacmes
          public    exebad,hexerr,exewrt,hexwrt,wrtmes1,wrtmes2
          public    execemes, ptymes
          extrn     rdflg:byte

badver:   db        "Incorrect DOS version",13,10,"$"
endmes:   db        13,10,"Program terminated normally"
carret:   db        13,10,"$"
nambad:   db        "Invalid drive specification",13,10,"$"
notfnd:   db        "File not found",13,10,"$"
noroom:   db        "File creation error",13,10,"$"
nospace:  db        "Insufficient disk space",13,10,"$"

disk:     db        "Disk$"
wrtpro:   db        "Write protect$"
dskerr:   db        " error "
errtyp:   db        "reading drive "
drvlet:   db        "A",13,10,"$"
readm:    db        "read"
writm:    db        "writ"

toobig:   db        "Insufficient memory",13,10,"$"
synerr:   db        '^'
errmes:   db        " Error",13,10+80H
bacmes:   db        32,8+80h
exebad    label     byte
hexerr:   db        "Error in EXE or HEX file",13,10,"$"
exewrt    label     byte
hexwrt:   db        "EXE and HEX files cannot be written",13,10,"$"
wrtmes1:  db        "Writing $"
wrtmes2:  db        " bytes",13,10,"$"
execemes: db        "EXEC failure",13,10,"$"
accmes:   db        "Access denied",13,10,"$"
ptymes:   db        "Parity error or nonexistant memory error detected",13,10,"$"

ifdef     help
helpmsg:  db        "Runs Debug, a program testing and editing tool.", 13, 10, 13, 10
          db        "DEBUG [[drive:][path]filename [testfile-parameters]]", 13, 10, 13, 10
          db        "  [drive:][path]filename  Specifies the file you want to test.", 13, 10
          db        "  testfile-parameters     Specifies command-line information required by", 13, 10
          db        "                          the file you want to test.", 13, 10, 13, 10
          db        "After Debug starts, type ? to display a list of debugging commands.", 13, 10, "$"
olhelp:   db        "assemble     A [address]", 13, 10
          db        "compare      C range address", 13, 10
          db        "dump         D [range]", 13, 10
          db        "enter        E address [list]", 13, 10
          db        "fill         F range list", 13, 10
          db        "go           G [=address] [addresses]", 13, 10
          db        "hex          H value1 value2", 13, 10
          db        "input        I port", 13, 10
          db        "load         L [address] [drive] [firstsector] [number]", 13, 10
          db        "move         M range address", 13, 10
          db        "name         N [pathname] [arglist]", 13, 10
          db        "output       O port byte", 13, 10
          db        "proceed      P [=address] [number]", 13, 10
          db        "quit         Q", 13, 10
          db        "register     R [register]", 13, 10
          db        "search       S range list", 13, 10
          db        "trace        T [=address] [value]", 13, 10
          db        "unassemble   U [range]", 13, 10
          db        "write        W [address] [drive] [firstsector] [number]", 13, 10
;         db        "allocate expanded memory        XA [#pages]", 13, 10
;         db        "deallocate expanded memory      XD [handle]", 13, 10
;         db        "map expanded memory pages       XM [Lpage] [Ppage] [handle]", 13, 10
;         db        "display expanded memory status  XS", 13, 10
          db        "$"
endif

constend:

const     ends
          end
