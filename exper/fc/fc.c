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
 * This code is untested.
 * 
 * Actually, the binary file compare does work in its original context, but I
 * converted it to use DOSAPI instead of LIBC, and I don't know if that works.
 * ASCII file compare has not been implemented at all, but I decided to put
 * the code into revision control as it is.
 */

#include <dos.h>
#include <stdio.h>
#include <string.h>

const char *m_noargs    = "Required parameter missing\r\n",
           *m_args      = "Too many parameters\r\n",
           *m_jumble    = "Invalid combination of switches\r\n",
           *m_switch    = "Invalid switch - /",
           *m_synerr    = "Syntax error\r\n",
           *m_comparing = "Comparing files ",
           *m_and       = " and ",
           *m_colon     = ": ",
           *m_space     = " ",
           *m_found     = "Found ",
           *m_total     = " total mismatches.\r\n",
           *m_longer    = " is longer than ",
           *m_fullstop  = ".\r\n",
#ifdef HELP
           *m_help      = 
"Compares two files or sets of files and displays the differences between them.\r\n\r\n"
"FC [/A] [/C] [/L] [/LBn] [/N] [/T] [/W] [/nnnn] [drive1:][path1]filename1\r\n"
"  [drive2:][path2]filename2\r\n"
"FC /B [drive1:][path1]filename1 [drive2:][path2]filename2\r\n\r\n"
"  /A     Displays only first and last lines for each set of differences.\r\n"
"  /B     Performs a binary comparison.\r\n"
"  /C     Disregards the case of letters.\r\n"
"  /L     Compares files as ASCII text.\r\n"
"  /LBn   Sets the maximum consecutive mismatches to the specified number of\r\n"
"         lines.\r\n"
"  /N     Displays the line numbers on an ASCII comparison.\r\n"
"  /T     Does not expand tabs to spaces.\r\n"
"  /W     Compresses white space (tabs and spaces) for comparison.\r\n"
"  /nnnn  Specifies the number of consecutive lines that must match after a\r\n"
"         mismatch.\r\n";
#endif
           *m_crlf      = "\r\n";

#define L_FLAG 0x01
#define B_FLAG 0x02
#define A_FLAG 0x04
#define C_FLAG 0x08
#define N_FLAG 0x10
#define T_FLAG 0x20
#define W_FLAG 0x40
unsigned char mode;
unsigned linbufsiz;
unsigned consecs;

/*
 * It is clearer to use a macro every time we return this code, than to write
 * down a magic number for the same purpose.
 */
#define EXIT_SYNERR 1

/* Error code returned from MS-DOS. */
int dos_errno;

unsigned char flags;

/* The command line arguments, after they have been parsed. */
char srcmask[80], tgtmask[80];

/*
 * Table of relevant MS-DOS error messages.
 * 
 * The order does not actually matter (for compression purposes) but it must
 * end with 0x00 and "Unknown error" so that the "print error message" routine
 * works correctly.
 */
struct
{
 unsigned char err;
 const char *msg;
} errtab[]={
 0x01, "Invalid function",
 0x02, "File not found",
 0x03, "Invalid path",
 0x04, "Too many open files",
 0x05, "Access denied",
 0x06, "Invalid handle",
 0x08, "Insufficient memory",
 0x09, "Invalid memory block address",
 0x0F, "Invalid drive specification",
 0x12, "No more files",
 0x13, "Disk write protected",
 0x14, "Unknown unit",
 0x15, "Drive not ready",
 0x16, "Unknown command",
 0x17, "Data CRC error",
 0x19, "Disk seek error",
 0x1A, "Invalid media type",
 0x1B, "Sector not found",
 0x1D, "Write fault",
 0x1E, "Read fault",
 0x1F, "Disk I/O error",
 0x20, "Share violation",
 0x21, "Lock violation",
 0x22, "Invalid media change",
 0x24, "Share buffer overrun",
 0x26, "Input past end of file",
 0x27, "Insufficient disk space",
 0x50, "File already exists",
 0x53, "Fail on INT 24",
 0x00, "Unknown error"
};

int dos_open (const char *filename, char mode)
{
 int r;

 _asm mov ah, 0x3D;
 _asm mov al, mode;
 _asm mov dx, filename;
 _asm int 0x21;
 _asm mov r, ax;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  dos_errno=r;
  return 0;
 }
 dos_errno=0;
 return r;
}

int dos_close (int handle)
{
 _asm mov ah, 0x3E;
 _asm mov bx, handle;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return -1;
 }
 dos_errno=0;
 return 0;
}

int dos_read (int handle, void *buf, int len)
{
 int r;
 
 _asm mov ah, 0x3F;
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

long dos_lseek (int handle, long pos, char mode)
{
 short a, b;
 long c;
 a=(pos>>16);
 b=(pos&0xFFFF);
 
 _asm mov ah, 0x42;
 _asm mov al, mode;
 _asm mov bx, handle;
 _asm mov cx, a;
 _asm mov dx, b;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return -1;
 }
 _asm mov a, dx;
 _asm mov b, ax;
 c=a;
 c<<=16;
 c|=b;
 return c;
}

int dos_getc (int handle)
{
 int x;
 char buf;
 
 x=dos_read(handle, &buf, 1);
 if (!x) return -1;
 return buf;
}

/*
 * Functions for console I/O, so we do not need to bring in big ugly printf().
 * (It makes the code a little uglier, but makes the binary significantly
 *  lighter, which may make the difference on a resource-constrained MS-DOS 
 *  machine.)
 */
void dos_putc (char c)
{
 _asm mov ah, 0x02;
 _asm mov dl, c;
 _asm int 0x21;
}

void dos_puts (const char *s)
{
 dos_write(1, s, strlen(s));
}

/*
 * Hex output variants:
 *   respectively 0=nibble, 1=byte, 2=word, 4=double word.
 */
void wrhex0 (unsigned char x)
{
 x &= 0x0F;
 
 dos_putc((x<10)?(x|'0'):(x+('A'-10)));
}

void wrhex1 (unsigned char x)
{
 wrhex0(x>>4);
 wrhex0(x);
}

void wrhex2 (unsigned short x)
{
 wrhex1(x>>8);
 wrhex1(x&0xFF);
}

void wrhex4 (unsigned long x)
{
 wrhex2(x>>16);
 wrhex2(x&0xFFFF);
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

unsigned char hiuprtab[128]={
 'C',  'U',  'E',  'A',  'A',  'A',  'A',  'C',
 'E',  'E',  'E',  'I',  'I',  'I',  'A',  'A',
 'E',  'A',  'A',  'O',  'O',  'O',  'U',  'U',
 'Y',  'O',  'U',  '$',  '$',  '$',  '$',  '$',
 'A',  'I',  'O',  'U',  'N',  'N',  0xA6, 0xA7,
 '?',  0xA9, 0xAA, 0xAB, 0xAC, '!',  '"',  '"',
 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
 'S',  0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/*
 * Watcom C:
 *   Wot, no peekb()? __m(@u@)m__
 */
unsigned char peekb (unsigned int seg, unsigned int off)
{
 return *((unsigned char *)(MK_FP(seg, off)));
}

void inittab (void)
{
 unsigned short dosver;
 unsigned char far *nlsptr;
 unsigned char tank[5], *tankptr;
 unsigned short buildptr[2];
 int t;
 
 _asm mov ah, 0x30;
 _asm int 0x21;
 _asm xchg ah, al;
 _asm mov dosver, ax;

 /*
  * Although we CAN get an uppercasing table in MS-DOS 2.11, the code to do so
  * is ugly and doesn't work very well in C (see SORT.A86 for an example), so
  * we just ignore this and use our internal table.  On 3.3 and later, we
  * get the table and overwrite our own with it.  It's still pretty ugly.
  */
 if (dosver<0x031E) return; /* use internal table */
  
 tankptr=tank;
 
 _asm mov ax, 0x6502;
 _asm mov cx, 0x0005;
 _asm mov di, tankptr;
 _asm mov bx, 0xFFFF;
 _asm mov dx, 0xFFFF;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1) return;
 buildptr[0]=tank[3];
 buildptr[0]<<=8;
 buildptr[0]|=tank[2];
 buildptr[1]=tank[1];
 buildptr[1]<<=8;
 buildptr[1]|=tank[0];
 nlsptr=MK_FP(buildptr[0], buildptr[1]);
 _asm mov buildptr[0], si;
 _asm mov buildptr[1], ds;
 for (t=0; t<128; t++) hiuprtab[t]=peekb(buildptr[1], buildptr[0]+t);
}

/* Case smash input. */
char smash (char x)
{
 if (x|0x80) return hiuprtab[x];
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
}

int strcmpfc (char *s1, char *s2, int crush)
{
 if (!crush) crush=mode;
 
 while (1)
 {
  char a, b;
  
  a=*s1;
  b=*s2;
  if (a&&(!b)) return 1;
  if ((!a)&&b) return -1;
  if ((!a)&&(!b)) return 0; /* equal */
  if (crush&W_FLAG) /* smash whitespace */
  {
   while ((a==' ')||(a=='\t')) a=*++s1;
   while ((b==' ')||(b=='\t')) b=*++s2;
  }
  if (crush&C_FLAG) /* smash case */
  {
   a=smash(a);
   b=smash(b);
  }
  if (a!=b) return a-b;
  s1++;
  s2++;
 }
}

int getswitch (char a)
{
 switch (a)
 {
#ifdef HELP
  case '?':
   dos_puts(m_help);
   exit(1);
#endif
  case 'A':
   mode|=A_FLAG;
   break;
  case 'B':
   mode|=B_FLAG;
   break;
  case 'C':
   mode|=C_FLAG;
   break;
  case 'L':
   mode|=L_FLAG;
   break;
  case 'N':
   mode|=N_FLAG;
   break;
  case 'T':
   mode|=T_FLAG;
   break;
  case 'W':
   mode|=W_FLAG;
   break;
  default:
   dos_puts(m_switch);
   dos_putc(a);
   dos_puts(m_crlf);
   return -1;
 }

 return 0;
}

/* Generic error printer. (Call as soon as possible after the error.) */
void wrerr (void)
{
 int p;
 
 for (p=0; errtab[p].err; p++)
 {
  if (errtab[p].err==dos_errno) break;
 }
 dos_puts(errtab[p].msg);
 dos_puts(m_crlf);
}

int bfc (char *filename1, char *filename2)
{
 int file1, file2;
 long l1, l2, e, t;
 int a, b, r;

 r=0;
 
 file1=dos_open(filename1,0);
 if (!file1)
 {
  wrerr();
  return 1;
 }
 
 file2=dos_open(filename2,0);
 if (!file2)
 {
  wrerr();
  dos_close(file1);
  return 1;
 }
 
 l1=dos_lseek(file1,0,2);
 l2=dos_lseek(file2,0,2);
 dos_lseek(file1,0,0);
 dos_lseek(file2,0,0);
 e=((l1>l2)?l2:l1);
 dos_puts (m_comparing);
 dos_puts (filename1);
 dos_puts (m_and);
 dos_puts (filename2);
 dos_puts (m_crlf);
 for (t=0; t<e; t++)
 {
  a=dos_getc(file1);
  b=dos_getc(file2);
  if (a<0)
  {
   wrerr();
   dos_close(file1);
   dos_close(file2);
   return 1;
  }
  if (b<0)
  {
   wrerr();
   dos_close(file1);
   dos_close(file2);
   return 1;
  }
  if (a!=b)
  {
   r++;
   wrhex4(t);
   dos_puts(m_colon);
   wrhex1(a);
   dos_puts(m_space);
   wrhex1(b);
  }
 }
 dos_close(file1);
 dos_close(file2);
 if (!((r==0)&&(l1!=l2)))
 {
  dos_puts (m_found);
  wrnum(r);
  dos_puts (m_total);
 }
 if (l1<l2)
 {
  dos_puts (filename1);
  dos_puts (m_longer);
  dos_puts (filename2);
  dos_puts (m_fullstop);
  return 2;
 }
 if (l1>l2)
 {
  dos_puts (filename2);
  dos_puts (m_longer);
  dos_puts (filename1);
  dos_puts (m_fullstop);
  return 2;
 }
 return (r)?3:0;
}

/*
 * Source: MS-DOS 6.22 and PC DOS 7 helpfiles.
 * 
 * If the file has one of these 6 extensions, default to a binary compare;
 * otherwise default to an ASCII compare.  This can be overridden with /B or
 * /L on the command line.
 */
int heuristic_ascbin (char *arg)
{
 if ((!strcmpfc(&(arg[strlen(arg)-4]), ".BIN", C_FLAG))||
     (!strcmpfc(&(arg[strlen(arg)-4]), ".COM", C_FLAG))||
     (!strcmpfc(&(arg[strlen(arg)-4]), ".EXE", C_FLAG))||
     (!strcmpfc(&(arg[strlen(arg)-4]), ".LIB", C_FLAG))||
     (!strcmpfc(&(arg[strlen(arg)-4]), ".OBJ", C_FLAG))||
     (!strcmpfc(&(arg[strlen(arg)-4]), ".SYS", C_FLAG)))
  return B_FLAG;
 return L_FLAG;
}

int main (int argc, char **argv)
{
 char *targ;
 int t;
 int nfb; /* Not for binary */
 
 /* Defaults */
 linbufsiz=100;
 consecs=2;
 nfb=0;
 
 /* Initialize uppercase table */
 inittab();
 
 /* Scream if no argument specified. */
 if (argc==1)
 {
  dos_puts(m_noargs);
  return EXIT_SYNERR;
 }
 
 /*
  * Traipse through the argument list DOS-style.
  * Note that this diddles argv, usually a Very Bad Thing(R)(TM)(C), but it
  * makes our parsing easier.
  */
 for (t=1; t<argc; t++)
 {
  char *ptr;
  
  ptr=strchr(argv[t], '/');
  if (ptr)
  {
   *ptr=0;
   ptr++;
   if (((*ptr)>='0')&&((*ptr)<='9'))
   {
    nfb=1;
    consecs=0;
    
    while (((*ptr)>='0')&&((*ptr)<='9'))
    {
     consecs=((*ptr)-'0')+(consecs*10);
     ptr++;
    }
    ptr--;
   }
   else if ((smash(*ptr)=='L')&&(smash(ptr[1])=='B'))
   {
    nfb=1;
    linbufsiz=0;
    
    ptr+=2;
    while (((*ptr)>='0')&&((*ptr)<='9'))
    {
     linbufsiz=((*ptr)-'0')+(linbufsiz*10);
     ptr++;
    }
    ptr--;
   }
   else if (getswitch(smash(*ptr)))
   {
    return 1;
   }
   ptr++;
   if (!*ptr) continue;
   while (*ptr)
   {
    if (*ptr!='/')
    {
     dos_puts(m_synerr);
     return 1;
    }
    ptr++;
    if (getswitch(smash(*ptr))) return 1;
    ptr++;
   }
  }
  
  /* If the beginning of the argument was /, it is now a NUL terminator. */
  if (!*argv[t]) continue;
  
  /*
   * If it is, in fact, one of our arguments, mark it.
   * Count them, and if there are too many, then die screaming.
   */
  if (targ==srcmask)
  {
   targ=tgtmask;
   strcpy(srcmask, argv[t]);
  }
  else if (targ==tgtmask)
  {
   targ=0;
   strcpy(tgtmask, argv[t]);
  }
  else
  {
   dos_puts(m_args);
   return EXIT_SYNERR;
  }
 }
 
 if (!(mode&(B_FLAG|L_FLAG))) mode |= heuristic_ascbin(srcmask);
 
 if (mode&B_FLAG)
 {
  if ((mode!=B_FLAG)||nfb)
  {
   dos_puts(m_jumble);
   return EXIT_SYNERR;
  }
  return bfc(srcmask, tgtmask);
 }
 
 dos_puts ("ASCII compare not yet implemented\r\n");

 return 0;
}
