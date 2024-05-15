; Copyright 1986-1988
;     International Business Machines Corp. & Microsoft Corp.
; Copyright 2024 S. V. Nickolas.
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
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT,TORT OR OTHERWISE, ARISING FROM,
; OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.

; ============================================================================
; uso: This reflects a sort of 3.3/4.0 hybrid version of DRIVER.SYS
;      It is the 4.0 version with the 3.3 command line parsing.
;      There are some oddities in the conversion.
; ============================================================================

;	SCCSID = @(#)driver.asm 4.13 85/10/15
;
; External block device driver
; Hooks into existing routines in IBMBIO block driver via Int 2F mpx # 8.
; This technique minimizes the size of the driver.

; Revised Try_h: to test for flagheads  as msg was being displayed on FormFactor
;  this caused the FormFactor to be set in the Head
; Revised the # of sectors/cluster for F0h to 1
;==============================================================================
;REVISION HISTORY:
;AN000 - New for DOS Version 4.00 - J.K.
;AC000 - Changed for DOS Version 4.00 - J.K.
;AN00x - PTM number for DOS Version 4.00 - J.K.
;==============================================================================
;AN001 - d55 Unable the fixed disk accessibility of DRIVER.SYS.     7/7/87 J.K.
;AN002 - p196 Driver.sys does not signal init. failure		    8/17/87 J.K.
;AN003 - p267 "No driver letter..." message                         8/19/87 J.K.
;AN004 - p268 "Too many parameter..." message                       8/20/87 J.K.
;AN005 - p300 "Bad 1.44MB BPB information..."                       8/20/87 J.K.
;AN006 - p490 Driver should reject identical parms		    8/28/87 J.K.
;AN007 - p921 Parser.ASM problem				    9/18/87 J.K.
;AN008 - d493 New init request structure			    2/25/88 J.K.
;==============================================================================

code segment byte public
assume cs:code,ds:code,es:code

; The following depend on the positions of the various letters in SwitchList

flagdrive   equ     0004H
flagcyln    equ     0008H
flagseclim  equ     0010H
flagheads   equ     0020H
flagff	    equ     0040H

;AN000;
.list

;---------------------------------------------------
;
;	Device entry point
;
DSKDEV	LABEL	WORD
	DW	-1,-1			; link to next device
	DW	0000100001000000B	; bit 6 indicates DOS 3.20 driver
	DW	STRATEGY
	DW	DSK$IN
DRVMAX	DB	1

;
; Various equates
;
CMDLEN	equ	0			;LENGTH OF THIS COMMAND
UNIT	equ	1			;SUB UNIT SPECIFIER
CMD	equ	2			;COMMAND CODE
STATUS	equ	3			;STATUS
MEDIA	equ	13			;MEDIA DESCRIPTOR
TRANS	equ	14			;TRANSFER ADDRESS
COUNT	equ	18			;COUNT OF BLOCKS OR CHARACTERS
START	equ	20			;FIRST BLOCK TO TRANSFER
EXTRA	equ	22			;Usually a pointer to Vol Id for error 15
CONFIG_ERRMSG  equ     23		;AN009; To set this field to Non-zero
					;	to display "Error in CONFIG.SYS..."

PTRSAV	DD	0


STRATP PROC FAR

STRATEGY:
	MOV	WORD PTR CS:[PTRSAV],BX
	MOV	WORD PTR CS:[PTRSAV+2],ES
	RET

STRATP ENDP

DSK$IN:
	push	es
	push	bx
	push	ax
	les	bx,cs:[ptrsav]
	cmp	byte ptr es:[bx].cmd,0
	jnz	Not_Init
	jmp	DSK$INIT

not_init:
; Because we are passing the call onto the block driver in IBMBIO, we need to
; ensure that the unit number corresponds to the logical (DOS) unit number, as
; opposed to the one that is relevant to this device driver.
	mov	al,byte ptr cs:[DOS_Drive_Letter]
	mov	byte ptr es:[bx].UNIT,al
	mov	ax,0802H
	int	2fH
;
; We need to preserve the flags that are returned by IBMBIO. YUK!!!!!
;
	pushf
	pop	bx
	add	sp,2
	push	bx
	popf

exitp	proc	far
DOS_Exit:
	pop	ax
	POP	BX
	POP	ES
	RET				;RESTORE REGS AND RETURN
EXITP	ENDP

include msbds.inc			; include BDS structures

BDS	DW	-1			;Link to next structure
	DW	-1
	DB	1			;Int 13 Drive Number
	DB	3			;Logical Drive Letter
FDRIVE:
	DW	512			;Physical sector size in bytes
	DB	-1			;Sectors/allocation unit
	DW	1			;Reserved sectors for DOS
	DB	2			;No. allocation tables
	DW	64			;Number directory entries
	DW	9*40			;Number sectors (at 512 bytes ea.)
	DB	00000000B		;Media descriptor, initially 00H.
	DW	2			;Number of FAT sectors
	DW	9			;Sector limit
	DW	1			;Head limit
	DW	0			;Hidden sector count
	dw	0			;AN000; Hidden sector count (High)
	dw	0			;AN000; Number sectors (low)
	dw	0			;AN000; Number sectors (high)
	DB	0			; TRUE => Large fats
OPCNT1	DW	0			;Open Ref. Count
	DB	2			;Form factor
FLAGS1	DW	0020H			;Various flags
	DW	80			;Number of cylinders in device
RecBPB1 DW	512			; default is that of 3.5" disk
	DB	2
	DW	1
	DB	2
	DW	70h
	DW	2*9*80
	DB	0F9H
	DW	3
	DW	9
	DW	2
	DW	0
	dw	0			;AN000;
	dw	0			;AN000;
	dw	0			;AN000;
	db	6 dup (0)		;AC000;
TRACK1	DB	-1			;Last track accessed on this drive
TIM_LO1 DW	-1			;Keep these two contiguous (?)
TIM_HI1 DW	-1
VOLID1	DB	"NO NAME    ",0         ;Volume ID for this disk
VOLSER	dd	0			;AN000;
FILE_ID db	"FAT12   ",0            ;AN000;

DOS_Drive_Letter	db	?	; Logical drive associated with this unit

ENDCODE LABEL WORD			; Everything below this is thrown away
					; after initialisation.

DskDrv	    dw	    offset FDRIVE	; "array" of BPBs

;AN000; For system parser;

FarSW	equ	0	; Near call expected
DateSW	equ	0	; Check date format
TimeSW	equ	0	; Check time format
FileSW	equ	0	; Check file specification
CAPSW	equ	0	; Perform CAPS if specified
CmpxSW	equ	0	; Check complex list
NumSW	equ	1	; Check numeric value
KeySW	equ	0	; Support keywords
SwSW	equ	1	; Support switches
Val1SW	equ	1	; Support value definition 1
Val2SW	equ	1	; Support value definition 2
Val3SW	equ	0	; Support value definition 3
DrvSW	equ	0	; Support drive only format
QusSW	equ	0	; Support quoted string format
;---------------------------------------------------
;.xlist
assume ds:code					;AN007;
;.list
;Control block definitions for PARSER.
;---------------------------------------------------
Parms	label	byte
	dw	Parmsx		;AN000;
	db	0		;AN000;No extras

Parmsx	label	byte		;AN000;
	db	0,0		;AN000;No positionals
	db	5		;AN000;5 switch control definitions
	dw	D_Control	;AN000;/D
	dw	T_Control	;AN000;/T
	dw	HS_Control	;AN000;/H, /S
	dw	CN_Control	;AN000;/C, /N
	dw	F_Control	;AN000;/F
	db	0		;AN000;no keywords

D_Control	label	word	;AN000;
	dw	8000h		;AN000;numeric value
	dw	0		;AN000;no functions
	dw	Result_Val	;AN000;result buffer
	dw	D_Val		;AN000;value defintions
	db	1		;AN000;# of switch in the following list
Switch_D	label	byte	;AN000;
	db	'/D',0          ;AN000;

D_Val	label	byte		;AN000;
	db	1		;AN000;# of value defintions
	db	1		;AN000;# of ranges
	db	1		;AN000;Tag value when match
;	 dd	 0,255		 ;AN000;
	dd	0,127		;AN001;Do not allow a Fixed disk.

Result_Val	label	byte	;AN000;
	db	?		;AN000;
Item_Tag	label	byte	;AN000;
	db	?		;AN000;
Synonym_ptr	label	word	;AN000;
	dw	?		;AN000;es:offset -> found Synonym
RV_Byte 	label	byte	;AN000;
RV_Word 	label	word	;AN000;
RV_Dword	label	dword	;AN000;
	dd	?		;AN000;value if number, or seg:off to string

T_Control	label	word	;AN000;
	dw	8000h		;AN000;numeric value
	dw	0		;AN000;no functions
	dw	Result_Val	;AN000;result buffer
	dw	T_Val		;AN000;value defintions
	db	1		;AN000;# of switch in the following list
Switch_T	label	byte	;AN000;
	db	'/T',0          ;AN000;

T_Val	label	byte		;AN000;
	db	1		;AN000;# of value defintions
	db	1		;AN000;# of ranges
	db	1		;AN000;Tag value when match
	dd	1,999		;AN000;

HS_Control	label	word	;AN000;
	dw	8000h		;AN000;numeric value
	dw	0		;AN000;no function flag
	dw	Result_Val	;AN000;Result_buffer
	dw	HS_VAL		;AN000;value definition
	db	2		;AN000;# of switch in following list
Switch_H	label	byte	;AN000;
	db	'/H',0          ;AN000;
Switch_S	label	byte	;AN000;
	db	'/S',0          ;AN000;

HS_Val	 label	 byte		;AN000;
	db	1		;AN000;# of value defintions
	db	1		;AN000;# of ranges
	db	1		;AN000;Tag value when match
	dd	1,99		;AN000;

CN_Control	 label	 word	;AN000;
	dw	0		;AN000;no match flags
	dw	0		;AN000;no function flag
	dw	Result_Val	;AN000;no values returned
	dw	NoVal		;AN000;no value definition
;	 db	 2		 ;AN000;# of switch in following list
	db	1		;AN001;
Switch_C	label	byte	;AN000;
	db	'/C',0          ;AN000;
;Switch_N	 label	 byte	 ;AN000;
;	 db	 '/N',0          ;AN000;

Noval	db	0		;AN000;

F_Control	label	word	;AN000;
	dw	8000h		;AN000;numeric value
	dw	0		;AN000;no function flag
	dw	Result_Val	;AN000;Result_buffer
	dw	F_VAL		;AN000;value definition
	db	1		;AN000;# of switch in following list
Switch_F	label	byte	;AN000;
	db	'/F',0          ;AN000;

F_Val		label	byte	;AN000;
	db	2		;AN000;# of value definitions (Order dependent)
	db	0		;AN000;no ranges
	db	4		;AN000;# of numeric choices
F_Choices	label	byte	;AN000;
	db	1		;AN000;1st choice (item tag)
	dd	0		;AN000;0
	db	2		;AN000;2nd choice
	dd	1		;AN000;1
	db	3		;AN000;3rd choice
	dd	2		;AN000;2
	db	4		;AN000;4th choice
	dd	7		;AN000;7

;
; Sets ds:di -> BDS for this drive
;
SetDrive:
	push	cs
	pop	ds
	mov	di,offset BDS
	ret

;
; Place for DSK$INIT to exit
;
ERR$EXIT:
	MOV	AH,10000001B			   ;MARK ERROR RETURN
	lds	bx, cs:[ptrsav]
	mov	byte ptr ds:[bx.MEDIA], 0	   ;AN002; # of units
	mov	word ptr ds:[bx.CONFIG_ERRMSG], -1 ;AN009;Show IBMBIO error message too.
	JMP	SHORT ERR1

Public EXIT
EXIT:	MOV	AH,00000001B
ERR1:	LDS	BX,CS:[PTRSAV]
	MOV	WORD PTR [BX].STATUS,AX ;MARK OPERATION COMPLETE

RestoreRegsAndReturn:
	POP	DS
	POP	BP
	POP	DI
	POP	DX
	POP	CX
	POP	AX
	POP	SI
	jmp	dos_exit


drivenumb   db	    5
cyln	    dw	    80
heads	    dw	    2
ffactor     db	    2
slim	    dw	    9

Switches    dw	0

Drive_Let_Sublist	label	dword
	db     11	;AN000;length of this table
	db	0	;AN000;reserved
	dw	D_Letter;AN000;
D_Seg	dw	?	;AN000;Segment value. Should be CS
	db	1	;AN000;DRIVER.SYS has only %1
	db	00000000b ;AN000;left align(in fact, Don't care), a character.
	db	1	;AN000;max field width 1
	db	1	;AN000;min field width 1
	db	' '     ;AN000;character for pad field (Don't care).

D_Letter	db	"A"

wrstr:
	push	ax
	push	ds
	push	cs
	pop	ds
	mov	ah,9
	int	21h
	pop	ds
	pop	ax
	ret

          
DSK$INIT:
	PUSH	SI
	PUSH	AX
	PUSH	CX
	PUSH	DX
	PUSH	DI
	PUSH	BP
	PUSH	DS

	LDS	BX,CS:[PTRSAV]		;GET POINTER TO I/O PACKET

	MOV	AL,BYTE PTR DS:[BX].UNIT    ;AL = UNIT CODE
	MOV	AH,BYTE PTR DS:[BX].MEDIA   ;AH = MEDIA DESCRIP
	MOV	CX,WORD PTR DS:[BX].COUNT   ;CX = COUNT
	MOV	DX,WORD PTR DS:[BX].START   ;DX = START SECTOR

	LES	DI,DWORD PTR DS:[BX].TRANS

	PUSH	CS
	POP	DS

	ASSUME	DS:CODE

	cld
	push	cs			;AN000; Initialize Segment of Sub list.
	pop	[D_Seg] 		;AN000;

; check for correct DOS version
	mov	ah,30h
	int	21H

	xchg	ah, al
	cmp	ax, 0314h		; 3.2 or later?
	jae	GoodVer			; Yes, skip

BadDOSVer:
	 Mov	 dx,offset edosver
	 call	 wrstr
	 jmp	 err$exitj2		 ; do not install driver

GoodVer:
	mov	ax,0800H
	int	2fH			    ; see if installed
	cmp	al,0FFH
	jnz	err$exitj2		    ; do not install driver if not present
	lds	bx,[ptrsav]
	mov	si,word ptr [bx].count	    ; get pointer to line to be parsed
	mov	ax,word ptr [bx].count+2
	mov	ds,ax
	call	Skip_Over_Name		    ; skip over file name of driver
	mov	di,offset BDS		    ; point to BDS for drive
	push	cs
	pop	es			    ; es:di -> BDS
	Call	ParseLine
	jc	err$exitj2
	LDS	BX,cs:[PTRSAV]
	mov	al,byte ptr [bx].extra	; get DOS drive letter
	mov	byte ptr es:[di].DriveLet,al
	mov	cs:[DOS_Drive_Letter],al
	add	al,"A"
	mov	cs:[drvltr],al		; set up for printing final message
	call	SetDrvParms		; Set up BDS according to switches
	jnc argok
	push ds
	push cs
	pop ds
	mov dx, offset earg
	call wrstr
	pop ds
	jmp short err$exitj2
argok:
	mov	ah,8			; Int 2f multiplex number
	mov	al,1			; install the BDS into the list
	push	cs
	pop	ds			; ds:di -> BDS for drive
	mov	di,offset BDS
	int	2FH
	lds	bx,dword ptr cs:[ptrsav]
	mov	ah,1
	mov	cs:[DRVMAX],ah
	mov	byte ptr [bx].media,ah
	mov	ax,offset ENDCODE
	mov	word ptr [bx].TRANS,AX	    ; set address of end of code
	mov	word ptr [bx].TRANS+2,CS
	mov	word ptr [bx].count,offset DskDrv
	mov	word ptr [bx].count+2,cs

	push	dx
	push	cs
	pop	ds
	mov	si, offset Drive_Let_SubList  ;AC000;
	mov	dx,offset eloaded
	call	wrstr
	pop	dx
	jmp	EXIT

err$exitj2:
	stc
	jmp	err$exit

;
; Skips over characters at ds:si until it hits a `/` which indicates a switch
; J.K. If it hits 0Ah or 0Dh, then will return with SI points to that character.
Skip_Over_Name:
	call	scanblanks
loop_name:
	lodsb
	cmp	al,0Dh				;AN003;
	je	End_SkipName			;AN003;
	cmp	al,0Ah				;AN003;
	je	End_SkipName			;AN003;
	cmp	al,'/'
	jnz	loop_name
End_SkipName:					;AN003;
	dec	si			    ; go back one character
	RET

ParseLine:
	 push	 di
	 push	 ds
	 push	 si
	 push	 es
Next_Swt:
	 call	 ScanBlanks
	 lodsb
	 cmp	 al,'/'
	 jz	 getparm
	 cmp	 al,13		     ; carriage return
	 jz	 done_line
	 CMP	 AL,10		     ; line feed
	 jz	 done_line
	 cmp	 al,0		     ; null string
	 jz	 done_line
	 mov	 ax,-2		     ; mark error invalid-character-in-input
	 stc
	 jmp	 short exitparse

getparm:
	 call	 Check_Switch
	 mov	 cs:Switches,BX      ; save switches read so far
	 jnc	 Next_Swt
	 cmp	 ax,-1		     ; mark error number-too-large
	 stc
	 jz	 exitparse
	 mov	 ax,-2		     ; mark invalid parameter
	 stc
	 jmp	 short exitparse

done_line:
	 test	 cs:Switches,flagdrive	   ; see if drive specified
	 jnz	 okay
	 push	 dx
	 mov	 dx,offset enodrive
	 call	 wrstr
	 pop	 dx
	 mov	 ax,-3		     ; mark error no-drive-specified
	 stc
	 jmp	 short exitparse

okay:
	 call	 SetDrive			; ds:di points to BDS now.
	 mov	 ax,cs:Switches
	 and	 ax,fChangeline+fNon_Removable	; get switches for Non_Removable and Changeline
	 or	 ds:[di].flags,ax
	 xor	 ax,ax		     ; everything is fine

;
; Can detect status of parsing by examining value in AX.
;	 0  ==>  Successful
;	 -1 ==>  Number too large
;	 -2 ==>  Invalid character in input
;	 -3 ==>  No drive specified

	 clc
exitparse:
	 pop	 es
	 pop	 si
	 pop	 ds
	 pop	 di
	 ret

;
; Scans an input line for blank or tab characters. On return, the line pointer
; will be pointing to the next non-blank character.
;
ScanBlanks:
	lodsb
	cmp	al,' '
	jz	ScanBlanks
	cmp	al,9		    ; Tab character
	jz	ScanBlanks
	dec	si
	ret

; Gets a number from the input stream, reading it as a string of characters.
; It returns the number in AX. It assumes the end of the number in the input
; stream when the first non-numeric character is read. It is considered an error
; if the number is too large to be held in a 16 bit register. In this case, AX
; contains -1 on return.

GetNum:
	 push	 bx
	 push	 dx
	 xor	 ax,ax
	 xor	 bx,bx
	 xor	 dx,dx

next_char:
	 lodsb
	 cmp	 al,'0'              ; check for valid numeric input
	 jb	 num_ret
	 cmp	 al,'9'
	 ja	 num_ret
	 sub	 al,'0'
	 xchg	 ax,bx		     ; save intermediate value
	 push	 bx
	 mov	 bx,10
	 mul	 bx
	 pop	 bx
	 add	 al,bl
	 adc	 ah,0
	 xchg	 ax,bx		     ; stash total
	 jc	 got_large
	 cmp	 dx,0
	 jz	 next_char
got_large:
	 mov	 ax,-1
	 jmp	 short get_ret

num_ret:
	 mov	 ax,bx
	 dec	 si		     ; put last character back into buffer

get_ret:
	 pop	 dx
	 pop	 bx
	 ret

; Processes a switch in the input. It ensures that the switch is valid, and
; gets the number, if any required, following the switch. The switch and the
; number *must* be separated by a colon. Carry is set if there is any kind of
; error.

Check_Switch:
	 lodsb
	 and	 al,0DFH	     ; convert it to upper case
	 cmp	 al,'A'
	 jb	 err_swtch
	 cmp	 al,'Z'
	 ja	 err_swtch
	 mov	 cl,cs:switchlist    ; get number of valid switches
	 mov	 ch,0
	 push	 es
	 push	 cs
	 pop	 es			 ; set es:di -> switches
	 push	 di
	 mov	 di,1+offset switchlist  ; point to string of valid switches
	 repne	 scasb
	 pop	 di
	 pop	 es
	 jnz	 err_swtch
	 mov	 ax,1
	 shl	 ax,cl		 ; set bit to indicate switch
	 mov	 bx,cs:switches
	 or	 bx,ax		 ; save this with other switches
	 mov	 cx,ax
	 test	 ax,7cH 	 ; test against switches that require number to follow
	 jz	 done_swtch
	 lodsb
	 cmp	 al,':'
	 jnz	 reset_swtch
	 call	 ScanBlanks
	 call	 GetNum
	 cmp	 ax,-1		 ; was number too large?
	 jz	 reset_swtch
	 call	 Process_Num

done_swtch:
	 ret

reset_swtch:
	 xor	 bx,cx			 ; remove this switch from the records
err_swtch:
	 stc
	 jmp	 short done_swtch

; This routine takes the switch just input, and the number following (if any),
; and sets the value in the appropriate variable. If the number input is zero
; then it does nothing - it assumes the default value that is present in the
; variable at the beginning.

Process_Num:
	 push	 ds
	 push	 cs
	 pop	 ds
	 test	 Switches,cx	     ; if this switch has been done before,
	 jnz	 done_ret	     ; ignore this one.
	 test	 cx,flagdrive
	 jz	 try_f
	 mov	 drivenumb,al
	 jmp	 short done_ret

try_f:
	 test	 cx,flagff
	 jz	 try_t
	 mov	 ffactor,al

try_t:
	 cmp	 ax,0
	 jz	 done_ret	     ; if number entered was 0, assume default value
	 test	 cx,flagcyln
	 jz	 try_s
	 mov	 cyln,ax
	 jmp	 short done_ret

try_s:
	 test	 cx,flagseclim
	 jz	 try_h
	 mov	 slim,ax
	 jmp	 short done_ret

; Switch must be one for number of Heads.
try_h:
	 test	 cx,flagheads
	 jz	 done_ret
	 mov	 heads,ax

done_ret:
	 pop	 ds
	 ret

; SetDrvParms sets up the recommended BPB in each BDS in the system based on
; the form factor. It is assumed that the BPBs for the various form factors
; are present in the BPBTable. For hard files, the Recommended BPB is the same
; as the BPB on the drive.
; No attempt is made to preserve registers since we are going to jump to
; SYSINIT straight after this routine.

SetDrvParms:
	push	cs
	pop	es
	xor	bx,bx
	call	SetDrive		; ds:di -> BDS
	;test	 cs:switches,flagff	 ; has formfactor been specified?
	;jz	 formfcont
	mov	bl,cs:[ffactor]
	mov	byte ptr [di].formfactor,bl   ; replace with new value
formfcont:
	mov	bl,[di].FormFactor
;AC000; The followings are redundanat since there is no input specified for Hard file.
;	 cmp	 bl,ffHardFile
;	 jnz	 NotHardFF
;	 mov	 ax,[di].DrvLim
;	 cmp	 ax, 0			 ;AN000;32 bit sector number?
;	 push	 ax
;	 mov	 ax,word ptr [di].hdlim
;	 mul	 word ptr [di].seclim
;	 mov	 cx,ax			 ; cx has # sectors per cylinder
;	 pop	 ax
;	 xor	 dx,dx			 ; set up for div
;	 div	 cx			 ; div #sec by sec/cyl to get # cyl
;	 or	 dx,dx
;	 jz	 No_Cyl_Rnd		 ; came out even
;	 inc	 ax			 ; round up
;No_Cyl_Rnd:
;	 mov	 cs:[cyln],ax
;	 mov	 si,di
;	 add	 si,BytePerSec		 ; ds:si -> BPB for hard file
;	 jmp	 short Set_RecBPB
;NotHardFF:
;AC000; End of deletion.
	cmp	bl,ff48tpi
	jnz	Got_80_cyln
	mov	cx,40
	mov	cs:[cyln],cx
Got_80_cyln:
	shl	bx,1			; bx is word index into table of BPBs
	mov	si,offset BPBTable
	mov	si,word ptr [si+bx]	; get address of BPB
Set_RecBPB:
	add	di,RBytePerSec		; es:di -> Recommended BPB
	mov	cx,BPBSIZ
	cld
	rep	movsb			; move BPBSIZ bytes

	call	Handle_Switches 	; replace with 'new' values as
					; specified in switches.

; We need to set the media byte and the total number of sectors to reflect the
; number of heads. We do this by multiplying the number of heads by the number
; of 'sectors per head'. This is not a fool-proof scheme!!

	mov	ax,[di].Rdrvlim 	; this is OK for two heads
	sar	ax,1			; ax contains # of sectors/head
	mov	cx,[di].Rhdlim
	dec	cl			; get it 0-based
	sal	ax,cl
	jc	Set_All_Done_BRG	; We have too many sectors - overflow!!
	mov	[di].Rdrvlim,ax
	cmp	cl,1

; We use media descriptor byte F0H for any type of medium that is not currently
; defined i.e. one that does not fall into the categories defined by media
; bytes F8H, F9H, FCH-FFH.

	JE	HEAD_2_DRV
	MOV	AL, 1				;1 sector/cluster
	MOV	BL, BYTE PTR [DI].Rmediad
	CMP	BYTE PTR [DI].FormFactor, ffOther
	JE	GOT_CORRECT_MEDIAD
	MOV	CH, BYTE PTR [DI].FormFactor
	CMP	CH, ff48tpi
	JE	SINGLE_MEDIAD
	MOV	BL, 0F0h
	JMP	GOT_CORRECT_MEDIAD
Set_All_Done_BRG:jmp Set_All_Done
SINGLE_MEDIAD:
	CMP	WORD PTR [DI].RSecLim, 8	;8 SEC/TRACK?
	JNE	SINGLE_9_SEC
	MOV	BL, 0FEh
	JMP	GOT_CORRECT_MEDIAD
SINGLE_9_SEC:
	MOV	BL, 0FCh
	JMP	GOT_CORRECT_MEDIAD
HEAD_2_DRV:
	MOV	BL, 0F0h		;default 0F0h
	MOV	AL, 1			;1 sec/cluster
	CMP	BYTE PTR [DI].FormFactor, ffOther
	JE	GOT_CORRECT_MEDIAD
	CMP	BYTE PTR [DI].FormFactor, ff48tpi
	JNE	NOT_48TPI
	MOV	AL, 2
	CMP	WORD PTR [DI].RSecLim, 8	;8 SEC/TRACK?
	JNE	DOUBLE_9_SEC
	MOV	BL, 0FFh
	JMP	GOT_CORRECT_MEDIAD
DOUBLE_9_SEC:
	MOV	BL, 0FDh
	JMP	GOT_CORRECT_MEDIAD
NOT_48TPI:
	CMP	BYTE PTR [DI].FormFactor, ff96tpi
	JNE	NOT_96TPI
	MOV	AL, 1			;1 sec/cluster
	MOV	BL, 0F9h
	JMP	GOT_CORRECT_MEDIAD
NOT_96TPI:
	CMP	BYTE PTR [DI].FormFactor, ffSmall	;3-1/2, 720kb
	JNE	GOT_CORRECT_MEDIAD	;Not ffSmall. Strange Media device.
	MOV	AL, 2			;2 sec/cluster
	MOV	BL, 0F9h

;J.K. 12/9/86 THE ABOVE IS A QUICK FIX FOR 3.3 DRIVER.SYS PROB. OLD LOGIC IS COMMENTED OUT.
;	 mov	 bl,0F0H		 ; assume strange media
;	 mov	 al,1			 ; AL is sectors/cluster - match 3.3 bio dcl. 6/27/86
;	 ja	 Got_Correct_Mediad
;; We check to see if the form factor specified was "other"
;	 cmp	 byte ptr [di].FormFactor,ffOther
;	 jz	 Got_Correct_Mediad
;; We must have 1 or 2 heads (0 is ignored)
;	 mov	 bl,byte ptr [di].Rmediad
;	 cmp	 cl,1
;	 jz	 Got_Correct_Mediad
;; We must have one head - OK for 48tpi media
;	 mov	 al,1			 ; AL is sectors/cluster
;	 mov	 ch,byte ptr [di].FormFactor
;	 cmp	 ch,ff48tpi
;	 jz	 Dec_Mediad
;	 mov	 bl,0F0H
;	 jmp	 short Got_Correct_Mediad
;Dec_Mediad:
;	 dec	 bl			 ; adjust for one head
;J.K. END OF OLD LOGIC

Got_Correct_Mediad:
	mov	byte ptr [di].RSecPerClus,al
	mov	byte ptr [di].Rmediad,bl
; Calculate the correct number of Total Sectors on medium
	mov	ax,word ptr [di].Ccyln
	mov	bx,word ptr [di].RHdLim
	mul	bx
	mov	bx,word ptr [di].RSecLim
	mul	bx
; AX contains the total number of sectors on the disk
	mov	word ptr [di].RDrvLim,ax
;J.K. For ffOther type of media, we should set Sec/FAT, and # of Root directory
;J.K. accordingly.
	cmp	byte ptr [di].FormFactor, ffOther  ;AN005;
	jne	Set_All_Ok			;AN005;
	xor	dx, dx				;AN005;
	dec	ax				;AN005; DrvLim - 1.
	mov	bx, 3				;AN005; Assume 12 bit fat.
	mul	bx				;AN005;  = 1.5 byte
	mov	bx, 2				;AN005;
	div	bx				;AN005;
	xor	dx, dx				;AN005;
	mov	bx, 512 			;AN005;
	div	bx				;AN005;
	inc	ax				;AN005;
	mov	[di].RCSecFat, ax		;AN005;
	mov	[di].RCDir, 0E0h		;AN005; directory entry # = 224
Set_All_Ok:					;AN005;
	clc
Set_All_Done:
	RET

;
; Handle_Switches replaces the values that were entered on the command line in
; config.sys into the recommended BPB area in the BDS.
; NOTE:
;	No checking is done for a valid BPB here.
;
Handle_Switches:
	call	setdrive		; ds:di -> BDS
	test	cs:switches,flagdrive
	jz	done_handle		    ; if drive not specified, exit
	mov	al,cs:[drivenumb]
	mov	byte ptr [di].DriveNum,al
;	 test	 cs:switches,flagcyln
;	 jz	 no_cyln
	mov	ax,cs:[cyln]
	mov	word ptr [di].cCyln,ax
no_cyln:
	test	cs:switches,flagseclim
	jz	no_seclim
	mov	ax,cs:[slim]
	mov	word ptr [di].RSeclim,ax
no_seclim:
	test	cs:switches,flagheads
	jz	done_handle
	mov	ax,cs:[heads]
	mov	word ptr [di].RHdlim,ax
done_handle:
	RET

; The following are the recommended BPBs for the media that we know of so
; far.

; 48 tpi diskettes

BPB48T	DW	512
	DB	2
	DW	1
	DB	2
	DW	112
	DW	2*9*40
	DB	0FDH
	DW	2
	DW	9
	DW	2
	DW	0

; 96tpi diskettes

BPB96T	DW	512
	DB	1
	DW	1
	DB	2
	DW	224
	DW	2*15*80
	DB	0F9H
	DW	7
	DW	15
	DW	2
	DW	0

BPBSIZ	=	$-BPB96T

; 3 1/2 inch diskette BPB

BPB35	DW	512
	DB	2
	DW	1			; Double sided with 9 sec/trk
	DB	2
	DW	70h
	DW	2*9*80
	DB	0F9H
	DW	3
	DW	9
	DW	2
	DW	0


BPBTable    dw	    BPB48T		; 48tpi drives
	    dw	    BPB96T		; 96tpi drives
	    dw	    BPB35		; 3.5" drives
; The following are not supported, so we default to 3.5" layout
	    dw	    BPB35		; Not used - 8" drives
	    dw	    BPB35		; Not Used - 8" drives
	    dw	    BPB35		; Not Used - hard files
	    dw	    BPB35		; Not Used - tape drives
	    dw	    BPB35		; Not Used - Other

switchlist  db	7,"FHSTDCN"         ; Preserve the positions of N and C.

edosver         db 0Ah                  ; DATA XREF: seg000:00FAo
                db 0Dh,'Incorrect DOS version',0Ah
                db 0Dh,'$'
earg            db 0Ah,0Dh,'Invalid parameter',0Ah
                db 0Dh,'$'
enodrive        db 0Ah
                db 0Dh,'No drive specified',0Ah
                db 0Dh,'$'
eloaded         db 'Loaded external disk driver for drive '; DATA XREF: seg000:016Co
drvltr          db 'A:',0Ah              ; DATA XREF: seg000:0137w
                db 0Dh,'$'

;AN000;
;Equates for message number
NODRIVE_MSG_NUM   equ	2
LOADOK_MSG_NUM	  equ	3

code ends

end
