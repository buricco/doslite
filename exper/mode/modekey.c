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
 * This contains the keyboard portion of the MODE command.
 * It has been architected for easy merging into a more complete MODE command.
 */
#include <stdlib.h>
#include <string.h>

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

const char *e_noargs = "Required parameter missing\r\n",
           *e_arg    = "Invalid parameter\r\n",
           *e_syntax = "Syntax error\r\n";

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

word getnum (char *x)
{
 word y;
 
 y=0;
 
 if ((*x<'0')||(*x>'9'))
 {
  dos_puts(e_arg);
  exit (1);
 }
 
 while ((*x>='0')&&(*x<='9'))
 {
  if (y>6552)
  {
   dos_puts(e_arg);
   exit (1);
  }
  
  y*=10;
  
  y+=((*x)&0x0F);
  x++;
 }
 
 return y;
}

int modekey (int argc, char **argv)
{
 int t;
 byte r, d;
 
 r=d=0xFF;
 
 if (argc!=4)
 {
  dos_puts(e_syntax);
  exit(1);
 }

 for (t=2; t<argc; t++)
 {
  if (strnicmp(argv[t], "RATE=", 5))
  {
   r=getnum(&(argv[t][5]));
   if ((r<1)||(r>32))
   {
    dos_puts(e_arg);
    exit(1);
   }
   r--;
  }
  else if (strnicmp(argv[t], "DELAY=", 6))
  {
   d=getnum(&(argv[t][6]));
   if ((d<1)||(d>4))
   {
    dos_puts(e_arg);
    exit(1);
   }
   d--;
  }
 }
 
 if ((r==0xFF)||(d==0xFF))
 {
  dos_puts(e_syntax);
  exit(1);
 }
 
 _asm mov ax, 0x0305;
 _asm mov bh, d;
 _asm mov bl, r;
 _asm int 0x16;
 
 return 0;
}

int main (int argc, char **argv)
{
 if (argc==1)
 {
  dos_puts(e_noargs);
  return 1;
 }
 
 if (strnicmp(argv[1], "CON", 3))
 {
  dos_puts(e_syntax);
  return 1;
 }
 
 return modekey (argc, argv);
}
