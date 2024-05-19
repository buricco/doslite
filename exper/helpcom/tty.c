/*
 * Copyright 2024 S. V. Nickolas.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the Software), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * This program includes a crude emulation of Microsoft's Character-Oriented
 * Windows library, limited to the extent of what is necessary to implement a
 * help engine.  The intention is that it is a near-exact workalike for the
 * MS-DOS 6 help engine, but without the weight of an entire TUI library and a
 * complete BASIC interpreter and IDE on top of it, such that it should be
 * able to run on an XT with PC DOS 3.3 and 256K RAM.
 * 
 * It isn't COW and doesn't include any of Microsoft's COW code.
 */

#include <dos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extern.h"

uint16_t vidseg;

int gotmouse;

int mos_check (void)
{
 int e;
 unsigned char far *p;
 uint32_t h;
 uint16_t s, o;
 _asm push es;
 _asm mov ax, 0x3533; /* Locate mouse interrupt handler */
 _asm int 0x21;
 _asm push es;
 _asm pop dx;
 _asm pop es;
 _asm mov s, dx;
 _asm mov o, bx;
 h=s;
 h<<=16;
 h|=o;
 p=(void far *)h;
 if (!p) return 0; /* BIOS initialized ISHs to 0000:0000 */
 if (*p==0xCF) return 0; /* BIOS or MS-DOS initialized ISHs to an IRET */
 
 _asm xor ax, ax;
 _asm int 0x33;
 _asm mov e, ax;
 return e;
}

void mos_csron (void)
{
 _asm mov ax, 0x0001;
 _asm int 0x33;
}

void mos_csroff (void)
{
 _asm mov ax, 0x0002;
 _asm int 0x33;
}

void mos_read (MOS_STAT *p)
{
 uint16_t m, x, y;
 
 _asm mov ax, 0x0003;
 _asm int 0x33;
 _asm mov m, bx;
 _asm mov x, cx;
 _asm mov y, dx;
 p->btnmask=m;
 p->x=x;
 p->y=y;
}

void mos_move (uint16_t x, uint16_t y)
{
 _asm mov ax, 0x0004;
 _asm mov cx, x;
 _asm mov dx, y;
 _asm int 0x33;
}

uint8_t vid_setmode (uint8_t mode)
{
 uint8_t e;
 
 _asm {
  xor ah, ah;
  mov al, mode;
  int 0x10;
  mov e, al;
 }
 return e;
}

void vid_gotoxy (uint8_t x, uint8_t y)
{
 _asm {
  mov dh, y;
  mov dl, x;
  xor bh, bh;
  mov ah, 0x02;
  int 0x10;
 }
}

/*
 * Special codes:
 *   c=-7 - black on light grey, ignore &
 *   c=-8 - light grey on black, & = accelerator
 *   c=-9 - black on light grey, & = accelerator
 *   c<0 otherwise - color parsing
 *   \atext\vlink\v - hyperlink (\v content currently ignored)
 *   \btext\p - bold
 */
void vid_wrstrat (uint8_t *s, uint8_t x, uint8_t y, int c)
{
 int v;
 uint8_t far *scrptr;
 uint8_t *txtptr;
 uint8_t a;
 
 uint16_t off;
 off=((y*80)+x)<<1;
 scrptr=MK_FP(vidseg, off);
 v=0;
 if (c==-7)
 {
  *(scrptr++)=' ';
  *(scrptr++)=0x70;
  for (txtptr=s; *txtptr; txtptr++)
  {
   if (*txtptr!='&')
   {
    *(scrptr++)=*txtptr;
    *(scrptr++)=0x70;
   }
  }
  *(scrptr++)=' ';
  *(scrptr++)=0x70;
  return;
 }
 if (c==-8)
 {
  int flag;
  
  flag=0;
  *(scrptr++)=' ';
  *(scrptr++)=0x07;
  for (txtptr=s; *txtptr; txtptr++)
  {
   if (*txtptr=='&')
   {
    flag=1;
   }
   else
   {
    *(scrptr++)=*txtptr;
    *(scrptr++)=(flag?0x0F:0x07);
    flag=0;
   }
  }
 }
 if (c==-9)
 {
  int flag;
  
  flag=0;
  *(scrptr++)=' ';
  *(scrptr++)=0x07;
  for (txtptr=s; *txtptr; txtptr++)
  {
   if (*txtptr=='&')
   {
    flag=1;
   }
   else
   {
    *(scrptr++)=*txtptr;
    *(scrptr++)=(flag?0x7F:0x70);
    flag=0;
   }
  }
  *(scrptr++)=' ';
  *(scrptr++)=0x70;
  return;
 }
 if (c<0) a=0x07; else a=c;
 for (txtptr=s; *txtptr; txtptr++)
 {
  if (*txtptr=='\\')
  {
   txtptr++;
   switch (*txtptr)
   {
    case 'b':
     if (c<0) a|=0x08;
     break;
    case 'p':
     if (c<0) a&=0xF7;
     break;
    case 'v':
     v=!v;
     break;
    case '\\':
     if (v) break;
     *(scrptr++)=*txtptr;
     *(scrptr++)=a;
     break;
    default:
     break;
   }
  }
  else
  {
   if (!v)
   {
    *(scrptr++)=*txtptr;
    if (c<0)
    {
     uint8_t k;
     
     k=*txtptr;
     if ((k==16)||(k==17)||(k=='<')||(k=='>'))
      *(scrptr++)=0x02;
     else
      *(scrptr++)=a;
    }
    else
     *(scrptr++)=a;
   }
  }
 }
}

void vid_wrchrat (uint8_t k, uint8_t x, uint8_t y, uint8_t c)
{
 uint8_t far *scrptr;
 
 uint16_t off;
 off=((y*80)+x)<<1;
 scrptr=MK_FP(vidseg, off);
 *scrptr=k;
 scrptr[1]=c;
}

/*
 * Interface to INT16
 *   key_read:  Read keyboard
 *   key_poll:  Is a key waiting?  Return it, or -1 if not.
 */
uint16_t key_read (void)
{
 uint16_t e;
 
 _asm {
  xor ah, ah;
  int 0x16;
  mov e, ax;
 }
 return e;
}

int32_t key_poll (void)
{
 uint8_t f;
 uint16_t e;
 
 f=0;
 _asm {
  mov ah, 0x01;
  int 0x16;
  jnz lkp_1;
  mov e, ax;
  jmp short lkp_2;
lkp_1:
  mov ah, 0xFF;
  mov f, ah;
lkp_2:
 }
 return f;
}
