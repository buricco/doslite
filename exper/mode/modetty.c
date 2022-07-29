/*
 * Copyright 2022 S. V. Nickolas.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
 */

/*
 * This almost works.
 * 
 * The calls to INT10, AH=$12, BL=$30 don't seem to be doing what they are
 * supposed to, resulting in ",43" setting 50 scanlines on VGA.
 * 
 * Remember that it is very common for x86 ASM to zero a register by XORing it
 * with itself (it is more efficient, especially for 16-bit registers, to do
 * this).
 */
#include <stdlib.h>
#include <string.h>

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

const char *e_arg    = "Invalid parameter\r\n",
           *e_syntax = "Syntax error\r\n",
           *e_cga    = "Function not supported on this computer\r\n",
           *e_status = "Status for device CON:\r\n"
                       "======================\r\n",
           *e_cols   = "Columns=",
           *e_rows   = "Lines=",
           *e_crlf   = "\r\n";

/* Error code returned from MS-DOS. */
int dos_errno;

byte flags;

int dos_write (int handle, const void *buf, int len)
{
 int r;

 _asm mov ah, 0x40;
 _asm mov bx, handle;
 _asm mov cx, len;
 _asm mov dx, buf;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 0;
 }
 dos_errno=0;
 _asm mov r, ax;
 return r;
}

void dos_puts (const char *s)
{
 dos_write(1, s, strlen(s));
}

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
}

void wrnum (unsigned long x)
{
 char n[11];
 char *p;
 
 n[10]=0;
 p=n+9;
 
 while (x)
 {
  *(p--)='0'+(x%10);
  x/=10;
 }
 p++;
 dos_puts(p);
}

void setmode (char m)
{
 _asm xor ah, ah;
 _asm mov al, m;
 _asm int 0x10;
}

int ttyset (char *arg)
{
 char *p;
 byte m;
 word w;
 enum {A_DONTBOTHER, A_BW40, A_BW80, A_CO40, A_CO80, A_MONO} x;
 
 p=strchr(arg, ',');
 
 if (p)
  *(p++)=0;

  if (!*arg)                      x=A_DONTBOTHER;
  else if (!stricmp(arg, "BW40")) x=A_BW40;
  else if (!stricmp(arg, "CO40")) x=A_CO40;
  else if (!stricmp(arg, "40"))   x=A_CO40;
  else if (!stricmp(arg, "BW80")) x=A_BW80;
  else if (!stricmp(arg, "CO80")) x=A_CO80;
  else if (!stricmp(arg, "80"))   x=A_CO80;
  else if (!stricmp(arg, "MONO")) x=A_MONO;
  else
  {
   dos_puts(e_arg);
   exit(1);
  }

 switch (x)
 {
  case A_DONTBOTHER:
   _asm mov ah, 0x0F;
   _asm int 0x10;
   _asm mov m, al;
   break;
  case A_BW40:
   m=0;
   break;
  case A_BW80:
   m=2;
   break;
  case A_CO40:
   m=1;
   break;
  case A_CO80:
   m=3;
   break;
  case A_MONO:
   m=7;
   break;
 }
 
 if (p)
 {
  if (*p)
  {
   _asm mov ah, 0x12;
   _asm mov bx, 0xFF10;
   _asm mov cx, 0xFFFF;
   _asm int 0x10;
   _asm mov w, cx;
   if ((w==0xFFFF)||(m==7))
   {
    if (strcmp(p, "25"))
    {
     dos_puts(e_cga);
     exit(1);
    }
   }
   _asm mov ax, 0x1A00;
   _asm int 0x10;
   _asm mov flags, al;
   if (flags==0x1A) /* VGA */
   {
    if (!strcmp(p, "25"))
    {
     _asm mov ax, 0x1202;
     _asm mov bl, 0x30;
     _asm int 0x10;
     setmode(m);
     _asm xor bl, bl;
     _asm mov ax, 0x1114;
     _asm int 0x10;
    }
    if (!strcmp(p, "43"))
    {
     _asm mov ax, 0x1201;
     _asm mov bl, 0x30;
     _asm int 0x10;
     setmode(m);
     _asm xor bl, bl;
     _asm mov ax, 0x1112;
     _asm int 0x10;
    }
    if (!strcmp(p, "50"))
    {
     _asm mov ax, 0x1202;
     _asm mov bl, 0x30;
     _asm int 0x10;
     setmode(m);
     _asm xor bl, bl;
     _asm mov ax, 0x1112;
     _asm int 0x10;
    }
   }
   else /* EGA */
   {
    if (!strcmp(p, "50"))
    {
     dos_puts(e_cga);
     exit(1);
    }
    if (!strcmp(p, "25"))
    {
     _asm mov ax, 0x1201;
     _asm mov bl, 0x30;
     _asm int 0x10;
     setmode(m);
     _asm xor bl, bl;
     _asm mov ax, 0x1110;
     _asm int 0x10;
    }
    if (!strcmp(p, "43"))
    {
     setmode(m);
     _asm mov ax, 0x1201;
     _asm mov bl, 0x30;
     _asm int 0x10;
     setmode(m);
     _asm xor bl, bl;
     _asm mov ax, 0x1112;
     _asm int 0x10;
    }
   }
  }
 }
 else 
  setmode(m);

 return 0;
}

void ttystat (void)
{
 word w;
 byte cols, rows;

 _asm mov ah, 0x12;
 _asm mov bx, 0xFF10;
 _asm mov cx, 0xFFFF;
 _asm int 0x10;
 _asm mov w, cx;
 if (w==0xFFFF)
 {
  rows=25;
 }
 else
 {
  _asm mov ax, 0x0040
  _asm push es;
  _asm mov es, ax;
  _asm mov ah, es:[0x0084];
  _asm pop es;
  _asm mov rows, ah;
  rows++;
 }
 _asm mov ah, 0x0F;
 _asm int 0x10;
 _asm mov cols, ah;
 
 dos_puts(e_status);
 dos_puts(e_cols);
 wrnum(cols);
 dos_puts(e_crlf);
 dos_puts(e_rows);
 wrnum(rows);
 dos_puts(e_crlf);
}

int main (int argc, char **argv)
{
 if (argc>2)
 {
  dos_puts(e_syntax);
  return 1;
 }
 
 if (argc==1)
 {
  ttystat();
  
  return 0;
 }
 
 return ttyset(argv[1]);
}
