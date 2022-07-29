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
 * This contains the COM: portion of the MODE command, except for retry mode.
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

char nextarg[10];

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

char *gettok (char *ptr)
{
 char *n;
 memset(nextarg, 0, 10);
 n=nextarg;
 
 while ((*ptr)&&(*ptr!=','))
 {
  *(n++)=*(ptr++);
 }
 if (!*ptr) return 0;
 if (*ptr==',') ptr++;
 return ptr;
}

byte getbaud(char *x)
{
 if (strncmp(x,"11",2))
  return 0x00;
 if (strncmp(x,"15",2))
  return 0x20;
 if (strncmp(x,"30",2))
  return 0x40;
 if (strncmp(x,"60",2))
  return 0x60;
 if (strncmp(x,"12",2))
  return 0x80;
 if (strncmp(x,"24",2))
  return 0xA0;
 if (strncmp(x,"48",2))
  return 0xC0;
 if (strncmp(x,"96",2))
  return 0xE0;
 dos_puts(e_arg);
 exit(1);
 
 return 0; /* Placate compiler */
}

byte getparity(char *x)
{
 switch (smash(*x))
 {
  case 'N':
   return 0x00;
  case 'O':
   return 0x08;
  case 'E':
   return 0x18;
  default:
   dos_puts(e_arg);
   exit(1);
 }
 
 return 0; /* Placate compiler */
}

byte getdata(char *x)
{
 switch (*x)
 {
  case '5':
   return 0x00;
  case '6':
   return 0x01;
  case '7':
   return 0x02;
  case '8':
   return 0x03;
  default:
   dos_puts(e_arg);
   exit(1);
 }
 
 return 0; /* Placate compiler */
}

byte getstop(char *x)
{
 switch (*x)
 {
  case '1':
   return 0x00;
  case '2':
   return 0x04;
  default:
   dos_puts(e_arg);
   exit(1);
 }
 
 return 0; /* Placate compiler */
}

int modecom (int argc, char **argv)
{
 char *ptr;
 byte port, baud, parity, data, stop, built;
 int t;

 port=argv[1][3];
 if ((port<'1')||(port>'4'))
 {
  dos_puts(e_syntax);
  return 1;
 }
 port-='1';
 
 baud=0xA0;
 parity=0x00;
 stop=0x00;
 data=0x03;
 
 if ((argv[1][4]==':')&&(argv[1][5]))
 {
  ptr=gettok(&(argv[1][5]));
  baud=getbaud(nextarg);
  if (ptr)
  {
   ptr=gettok(ptr);
   parity=getparity(nextarg);
   if (ptr)
   {
    ptr=gettok(ptr);
    data=getdata(nextarg);
    if (ptr)
    {
     ptr=gettok(ptr);
     stop=getstop(nextarg);
    }
   }
  }
 }
 else
 {
  for (t=2; t<argc; t++)
  {
   if (!strnicmp(argv[t],"baud=",5)) baud=getbaud(&(argv[t][5]));
   if (!strnicmp(argv[t],"parity=",7)) parity=getparity(&(argv[t][7]));
   if (!strnicmp(argv[t],"data=",5)) data=getdata(&(argv[t][5]));
   if (!strnicmp(argv[t],"stop=",5)) stop=getstop(&(argv[t][5]));
  }
 }
 
 built=baud|parity|data|stop;
 
 _asm xor ah, ah;
 _asm xor dh, dh;
 _asm mov al, built;
 _asm mov dl, port;
 _asm int 0x14;
 return 0;
}

int main (int argc, char **argv)
{
 if (argc==1)
 {
  dos_puts(e_noargs);
  return 1;
 }
 
 if (strnicmp(argv[1], "COM", 3))
 {
  dos_puts(e_syntax);
  return 1;
 }
 
 return modecom (argc, argv);
}
