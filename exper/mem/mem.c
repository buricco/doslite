/*
 * Copyright 2022 S. V. Nickolas.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal with the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 *   1. Redistributions of source code must retain the above copyright notice, 
 *      this list of conditions and the following disclaimers.
 *   2. Redistributions in binary form must reproduce the above copyright 
 *      notice, this list of conditions and the following disclaimers in the 
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products 
 *      derived from this Software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
 * 
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * MISSING FEATURES:
 *   * Detect total extended memory (not XMS) after HIMEM diddles int15
 *   * The switches don't do everything they're supposed to, though they do a
 *       fair bit now (based on PC DOS 4, more or less; less more, more less).
 *     PC DOS 4 had /PROGRAM and /DEBUG, MS-DOS 5 added /CLASSIFY.
 *     MS-DOS 6 did some major reworking of the switches but kept /CLASSIFY
 *       and /DEBUG.
 *   * Anything from MS-DOS 5 or later, except for HIDOS detection.
 */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

byte mode;

void far *farptr (word segment, word offset)
{
 dword fptr;
 
 fptr=segment;
 fptr<<=16;
 fptr|=offset;
 return (void far*) fptr;
}

void mem_main (void)
{
 byte far *foo;
 word mcbseg;
 word ourpsp;
 char block[9];
 dword genfree;
 dword chains[256]; /* should be more than enough */
 byte chaindirty;   /* also used for the flags when doing memory probes */
 int curchain;
 dword biggest;
 dword wasfound;
 word kb;
 word realkb;
 word ebdaseg;
 byte probefurther;
 byte hidos;
 dword xmsentry;
 byte emsfound;
 byte *emsdriv="EMMXXXX0";
 word lo, hi;
 void (far *memmgr)(void);
 
 word int15mem, int15free;
 word xmsmem, xmsfree;
 word emsmem, emsfree;
 
 xmsmem=xmsfree=0;
 emsmem=emsfree=0;
 
 _asm mov ah, 0x3D;           /* OPEN */
 _asm mov dx, emsdriv;
 _asm int 0x21;
 _asm push ax;                /* Tank the handle on the stack */
 _asm lahf;
 _asm mov emsfound, ah;       /* EMS is found if CARRY not set */
 emsfound=!(emsfound&1);
 _asm pop bx;                 /* Pick the handle up again */
 _asm mov ah, 0x3E;           /* CLOSE */
 _asm int 0x21;
 
 if (emsfound)
 {
#if 0 /* this hangs DOSBOX */
  /* Bug in MS-DOS 6.0's EMM386: make sure EMM386 is active */
  _asm mov ax, 0xFFA5;         /* ISEMM386 */
  _asm int 0x67;
  _asm mov lo, ax;
  if ((lo==0x845A)||(lo==0x84A5))
  {
   _asm mov lo, cx;
   _asm mov hi, bx;
   memmgr=farptr(hi, lo);
   if (memmgr)
   {
    _asm xor ah, ah;
    memmgr();
    _asm mov emsfound, al;
    emsfound=!(emsfound&0x01);
   }
  }
#endif

  _asm mov ah, 0x42;
  _asm int 0x67;
  _asm mov chaindirty, ah;    /* 0=OK */
  if (!chaindirty)
  {
   _asm mov emsmem, dx;
   _asm mov emsfree, bx;
  }
 }
 
 /* Main memory */
 _asm int 0x12;
 _asm mov kb, ax;
 
 /* EBDA memory - PC DOS adds this to the total but displays both values */
 _asm push es;
 _asm mov ah, 0xC1;
 _asm int 0x15;
 _asm mov ebdaseg, es;
 _asm lahf;
 _asm mov chaindirty, ah;
 _asm pop es;
 if (chaindirty&1)
  realkb=kb;
 else
  realkb=kb+(*((byte far *)farptr(ebdaseg, 0)));
 
 /* INT15 extended memory */
 _asm mov ah, 0x88;
 _asm int 0x15;
 _asm mov int15mem, ax;
 _asm lahf;
 _asm mov chaindirty, ah;
 probefurther=1;
 if (chaindirty&1)
 {
  int15mem=0;
  probefurther=0;
 }
 
 if (probefurther)
 {
  byte gotxms;
  
  _asm mov ax, 0x4300;
  _asm int 0x2F;
  _asm mov gotxms, al;
  hi=lo=0;
  if (gotxms==0x80)
  {
   _asm push es;
   _asm mov ax, 0x4310;
   _asm int 0x2F;
   _asm mov lo, bx;
   _asm mov hi, es;
   _asm pop es;
   memmgr=farptr(hi,lo);
   
   _asm mov ah, 0x08;
   _asm xor bl, bl;
   if (memmgr)
   {
    memmgr();
    _asm mov xmsmem, dx;
    _asm mov xmsfree, ax;
   }
   else
   {
    xmsmem=xmsfree=0;
   }
  }
 }
 
 if (_osmajor>4)
 {
  _asm mov ax, 0x3306;
  _asm int 0x21;
  _asm mov hidos, dh;
  hidos=(hidos&0x10)?0xFF:0x00;
 }
 else
 {
  hidos=0;
 }
 
 _asm mov ourpsp, cs;
 
 _asm push es;
 _asm mov ah, 0x52;
 _asm int 0x21;
 _asm mov ax, es:[bx-2];
 _asm mov mcbseg, ax;
 _asm pop es;
 
 genfree=0;
 wasfound=0;
 chaindirty=curchain=0;

 if (mode)
  printf ("\n"
          "  Address       Name        Size\n"
          "  -------     --------     ------\n"); 

 while (1)
 {
  byte mz;
  word pspseg;
  word mempara;
  dword membyte;
  word memkbyte;
  byte isfree;
  byte far *filename;
  byte lastwasfree;
  
  foo=farptr(mcbseg, 0);
  mz=*foo;
  pspseg=*((word far*)(&foo[1]));
  mempara=*((word far*)(&foo[3]));
  filename=&(foo[8]);
 
  if ((mz!='M')&&(mz!='Z'))
  {
   fprintf (stderr, "Memory arena trashed\n");
   exit(1);
  }
  memset(block,0,9);
  if (_osmajor<4)
  {

  }
  else
  {
   byte i;
   
   for (i=0; ((i<8)&&filename[i]); i++) block[i]=filename[i];
  }

  isfree=0;
  *chains=0;
  
  wasfound+=mempara;
  wasfound++;
  
  if ((!pspseg)||(pspseg==ourpsp))
  {
   if (chaindirty)
   {
    chaindirty=0;
    curchain++;
    chains[curchain]=0;
   }
   isfree=1;
   chains[curchain]+=mempara;
   chains[curchain]++;
   strcpy(block, " -Free-");
  }
  else
  {
   chaindirty=1;
   if (pspseg==8)
    strcpy(block, "DOS");
  }
  
  if (mode)
   printf ("  0%04X0      %-8s     0%04X0     \n", pspseg, block, mempara);
  
  if ((pspseg==8)&&(mode==2))
  {
   void far *nextdev;
   byte far *devnam;
   byte curdrv;
   
   mode--; /* Don't do this twice. (RxDOS) */
   
   /* Get the first device header. */
   _asm push es;
   _asm mov ah, 0x52;
   _asm int 0x21;
   _asm mov lo, bx;
   _asm mov hi, es;
   _asm pop es;
   lo+=0x22;
   nextdev=farptr(hi, lo);
   curdrv=0;
   do
   {
    devnam=nextdev;
    if (devnam[5]&0x80)       /* Character device */
    {
     byte t;
     
     devnam+=0x0A;
     memset(block, 0, 9);
     for (t=0; t<8; t++) block[t]=devnam[t];
     for (t=7; t, (block[t]==' '); t--) block[t]=0;
     printf ("                 %s\n", block);
    }
    else                      /* Block device */
    {
     byte first, last;
     
     devnam+=0x0A;
     first=curdrv;
     curdrv+=*devnam;         /* Number of drives managed by this driver */
     last=curdrv-1;
     printf ("                 %c:", first+'A');
     if (*devnam!=1)
      printf (" - %c:", last+'A');
     printf ("\n");
    }
    
    lo=((word far *)nextdev)[0];
    hi=((word far *)nextdev)[1];
    
    nextdev=farptr(hi, lo);
   } while (lo!=0xFFFF);
  }
  
  membyte=mempara;
  membyte<<=4;
  memkbyte=mempara>>6;
  if (mempara&0x3F) memkbyte++;
  if (isfree)
  {
   genfree+=membyte;
  }
  if (mz=='Z') break;
  mcbseg++;
  mcbseg+=mempara;
 }
 biggest=0;
 while (curchain>0)
 {
  if (biggest<chains[curchain]) biggest=chains[curchain];
  curchain--;
 }
 biggest--;
 biggest<<=4;
 
 /* Summary */
 
 printf ("\n"
         "%10lu bytes total conventional memory\n", ((dword) realkb)<<10);
 printf ("%10lu bytes available to DOS\n", ((dword) kb)<<10);
 printf ("%10lu largest executable program size\n", biggest);
 
 if (emsfound)
 {
  printf ("%10lu bytes total EMS memory\n", ((dword) emsmem)<<14);
  printf ("%10lu bytes free EMS memory\n", ((dword) emsfree)<<14);
 }
 
 if (int15mem||probefurther)
 {
  /* XXX: if int15 has been diddled, how do we get the old value? */
  printf ("%10lu bytes available contiguous extended memory\n", ((dword) int15mem)<<10);
  if (xmsmem)
  {
   printf ("%10lu bytes available XMS memory\n", ((dword) xmsfree)<<10);
  }
 }
 if (hidos)
  printf ("           DOS is resident in the High Memory Area\n");
}

char *getswitch (char *cl)
{
 if (*cl=='\r') return cl;
#ifdef HELP
 if (!strnicmp(cl, "/?", 2))
 {
  printf (
"Displays the amount of used and free memory in your system.\r\n\r\n"
"MEM [/PROGRAM | /DEBUG | /CLASSIFY]\r\n\r\n"
"  /PROGRAM or /P   Displays status of programs currently loaded in memory.\r\n"
"  /DEBUG or /D     Displays status of programs, internal drivers, and other\r\n"
"                   information.\r\n"
  );
  exit(0);
 }
#endif
 if (!strnicmp(cl, "/PROGRAM", 8))
 {
  mode=1;
  cl+=8;
 }
 else if (!strnicmp(cl, "/DEBUG", 6))
 {
  mode=2;
  cl+=6;
 }
 else
 {
  return 0;
 }
 while ((*cl==' ')||(*cl=='\t')) cl++;
 return cl;
}

#pragma argsused
int main (int argc, char **argv)
{
 word dosver;
 char *cl;
 
 _asm mov ah, 0x30;
 _asm int 0x21;
 _asm xchg ah, al;
 _asm mov dosver, ax;
 
 if (dosver<0x030A)
 {
  fprintf (stderr, "Incorrect DOS version\n");
  return 1;
 }
 
 mode=0;
 
 cl=(void*) 0x0081;
 
 while ((*cl==' ')||(*cl=='\t')) cl++;

 cl=getswitch(cl);
 if (!cl)
 {
  fprintf (stderr, "Invalid parameter\n");
  return 1;
 }
 if (*cl!='\r')
 {
  fprintf (stderr, "Too many parameters\n");
  return 1;
 }
 
 mem_main();
 
 return 0;
}
