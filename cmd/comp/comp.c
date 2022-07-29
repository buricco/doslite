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
 * This program mostly works, but it has issues when wildcards are used in the
 * target mask (and wildcards in the source mask are untested).
 * 
 * It is not based on the enhanced DOS 5 version of COMP, but the PC DOS 2.0
 * version, which means no switches.
 * 
 * Much of the scaffolding was attempted in ASM, then rewritten in C when it
 * got too ugly.  That is the reason for all the "goto" statements.  I never
 * really agreed with Edsger Dijkstra anyway.
 */

#include <string.h>

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

const byte *e_args   = "Too many parameters\r\n",
           *e_ten    = "\r\n10 mismatches - terminating compare\r\n",
           *e_head   = "Comparing ",
           *e_and    = " and ",
           *e_diff   = "\r\nCompare error at offset ",
           *e_file1  = "File 1 = ",
           *e_file2  = "File 2 = ",
           *e_noeof  = "EOF mark not found\r\n",
           *e_ok     = "Files compare OK\r\n",
           *e_dash   = " - ",
           *e_none   = " - No files found\r\n",
           *e_first  = "\r\nEnter primary file name\r\n",
           *e_second = "\r\nEnter second file name or drive ID\r\n",
           *e_size   = "Files are different sizes\r\n",
           *e_more   = "\r\nCompare more files (Y/N)? ",
#ifdef HELP
           *e_help   =
"Compares the contents of two files or sets of files.\r\n\r\n"
"COMP [data1] [data2]\r\n\r\n"
"  data1     Specifies location and name(s) of first file(s) to compare.\r\n"
"  data2     Specifies location and name(s) of second files to compare.\r\n\r\n"
"To compare sets of files, use wildcards in data1 and data2 parameters.\r\n",
#endif
           *e_crlf   = "\r\n";

char linbuf[131];

char fnbuf1[128], fnbuf2[128], fnbuf3[128], fnbuf4[128];

/*
 * MS-DOS file attributes.
 * We don't use them all, but define them anyway.
 */
#define ATTR_READONLY  0x01
#define ATTR_SYSTEM    0x02
#define ATTR_HIDDEN    0x04
#define ATTR_LABEL     0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

/* Error code returned from MS-DOS. */
int dos_errno;

byte flags;

/*
 * Struct used by FINDFIRST and FINDNEXT.
 * The definition of reserved[] is irrelevant and varies from version to
 * version according to RBIL so never mind, we're only using fnam anyway.
 */
struct dos_ffblk
{
 unsigned char reserved[21];
 unsigned char attr;
 unsigned long mtime;
 unsigned long size;
 unsigned char fnam[13];
};

struct dos_fcb
{
 byte  drive;
 byte  filename[8];
 byte  ext[3];
 word  block;
 word  recsiz;
 dword fsiz;
 dword timestamp;
 byte  reserved[8];
 byte  subrecord;
 dword randrec;
};

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

int dos_read (int handle, void *buf, unsigned len)
{
 int r;

 _asm mov ah, 0x3F;
 _asm mov bx, handle;
 _asm mov cx, len;
 _asm mov dx, buf;
 _asm int 0x21;
 _asm mov r, ax;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 0;
 }
 dos_errno=0;
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
 _asm mov r, ax;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 0;
 }
 dos_errno=0;
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
 _asm mov a, dx;
 _asm mov b, ax;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return -1;
 }
 c=a;
 c<<=16;
 c|=b;
 return c;
}

int dos_findfirst (const char *mask, int attr, struct dos_ffblk *f)
{
 _asm mov ah, 0x1A; /* SETDTA */
 _asm mov dx, f;
 _asm int 0x21;
 _asm mov ah, 0x4E;
 _asm mov cx, attr;
 _asm mov dx, mask;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 0;
 }
 dos_errno=0;
 return 1;
}

int dos_findnext (struct dos_ffblk *f)
{
 _asm mov ah, 0x1A; /* SETDTA */
 _asm mov dx, f;
 _asm int 0x21;
 _asm mov ah, 0x4F;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 0;
 }
 dos_errno=0;
 return 1;
}

int dos_getattr (const char *filename)
{
 int r;

 _asm mov ax, 0x4300;
 _asm mov dx, filename;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 0;
 }
 dos_errno=0;
 _asm mov r, cx;
 return r;
}

int dos_parsfnm (char *filename, struct dos_fcb *fcb, byte options)
{
 byte e;
 _asm mov ah, 0x29;
 _asm mov al, options;
 _asm mov si, filename;
 _asm mov di, fcb;
 _asm int 0x21;
 _asm mov e, al;
 return e;
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

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
}

void dos_getlin (char *myptr, byte len)
{
 byte *lb;
 byte t;
 
 lb=linbuf;
 
 *linbuf=len;
 linbuf[1]=0;
 _asm mov ah, 0x0A;
 _asm mov dx, lb;
 _asm int 0x21;
 dos_puts(e_crlf);
 for (t=0; (linbuf[t+2]!=0x0D)&&(t<len); t++)
  myptr[t]=linbuf[t+2];
 myptr[t]=0;
}

/*
 * Technically this relies on an overrun, but it should be acceptable.
 */
int findwild (struct dos_fcb fcb)
{
 int x;
 
 for (x=0; x<11; x++)
  if (fcb.filename[x]=='?') return 1;
  
 return 0;
}

/*
 * Generic error printer. (Call as soon as possible after the error.)
 */
void wrerr (void)
{
 int p;
 
 for (p=0; errtab[p].err; p++)
 {
  if (errtab[p].err==dos_errno) break;
 }
 dos_puts(errtab[p].msg);
 dos_puts(e_crlf);
}

char *basename (char *ptr)
{
 char *x;
 
 x=strrchr(ptr, '\\');
 if (x) return x+1;
 if (ptr[1]==':') return ptr+2;
 return ptr;
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

int do_it2 (char *source, char *target)
{
 byte tally;
 int c1, c2, e;
 int handle1, handle2;
 long l1, l2;
 
 dos_puts(e_head);
 dos_puts(source);
 dos_puts(e_and);
 dos_puts(target);
 dos_puts(e_crlf);
 dos_puts(e_crlf);
 
 e=0;
 tally=0;
 
 handle1=dos_open(source, 0);
 if (!handle1)
 {
  byte temp;
  
  temp=dos_errno;
  dos_puts(source);
  dos_puts(e_dash);
  dos_errno=temp;
  wrerr();
  return -1;
 }
 
 handle2=dos_open(target, 0);
 if (!handle2)
 {
  byte temp;
  
  temp=dos_errno;
  dos_close(handle1);
  dos_puts(target);
  dos_puts(e_dash);
  dos_errno=temp;
  wrerr();
  return -1;
 }
 
 l1=dos_lseek(handle1, 0, 2);
 l2=dos_lseek(handle2, 0, 2);
 dos_lseek(handle1, 0, 0);
 dos_lseek(handle2, 0, 0);
 if (l1!=l2)
 {
  dos_close(handle1);
  dos_close(handle2);
  dos_puts(e_size);
  return -1;
 }
 
 for (l2=0; l2<l1; l2++)
 {
  c1=dos_getc(handle1);
  if (c1<0)
  {
   byte temp;
  
   temp=dos_errno;
   dos_puts(source);
   dos_puts(e_dash);
   dos_errno=temp;
   wrerr();
   dos_close(handle1);
   dos_close(handle2);
   return -1;
  }
  
  c2=dos_getc(handle2);
  if (c2<0)
  {
   byte temp;
  
   temp=dos_errno;
   dos_puts(target);
   dos_puts(e_dash);
   dos_errno=temp;
   wrerr();
   dos_close(handle1);
   dos_close(handle2);
   return -1;
  }
  
  if (c1==0x1A) e=1;
  
  if (c1!=c2)
  {
   dos_puts(e_diff);
   wrhex4(l2);
   dos_puts(e_crlf);
   dos_puts(e_file1);
   wrhex1(c1);
   dos_puts(e_crlf);
   dos_puts(e_file2);
   wrhex1(c2);
   dos_puts(e_crlf);
   tally++;
   if (tally>=10)
   {
    e=1;
    dos_puts(e_ten);
    break;
   }
  }
 }
 dos_close(handle1);
 dos_close(handle2);
 
 if (!e) dos_puts(e_noeof);
 if (!tally) dos_puts(e_ok);
 
 return 0;
}

int do_it1 (char *source, char *tgtmask)
{
 char *ptr;
 char minibuf[13];
 int c, t;
 struct dos_fcb fcb1, fcb2;
 
 /* Just a drive letter */
 if (!strcmp(tgtmask+1, ":"))
 {
  strcpy(fnbuf4, tgtmask);
  strcat(fnbuf4, basename(source));
  return do_it2(source, fnbuf4);
 }
 
 /* Directory */
 if (dos_getattr(tgtmask) & ATTR_DIRECTORY)
 {
  strcpy(fnbuf4, tgtmask);
  if (fnbuf4[strlen(fnbuf4)-1]!='\\') strcat(fnbuf4, "\\");
  strcat(fnbuf4, basename(source));
  return do_it2(source, fnbuf4);
 }
 
 ptr=basename(tgtmask);
 dos_parsfnm(ptr, &fcb2, 0);
 if (!findwild(fcb2))
  return do_it2(source,tgtmask);
 
 /* Construct filename */
 ptr=basename(source);
 dos_parsfnm(ptr, &fcb1, 0);
 for (t=0; t<11; t++)
 {
  if (fcb2.filename[t]=='?') fcb2.filename[t]=fcb1.filename[t];
 }
 c=7;
 while (fcb2.filename[c]==' ') c--;
 c++;
 memset(minibuf, 0, 13);
 for (t=0; t<c; t++) minibuf[t]=fcb2.filename[t];
 strcat(minibuf, ".");
 ptr=minibuf+strlen(minibuf);
 c=2;
 while (c&&(fcb2.ext[c]==' ')) c--;
 if (!c)
  minibuf[strlen(minibuf)-1]=0;
 else
  for (t=0; t<=c; t++) *(ptr++)=fcb2.ext[t];
 
 strcpy(fnbuf4, tgtmask);
 ptr=basename(fnbuf4);
 *ptr=0;
 if (ptr!=fnbuf4) strcpy(ptr, "\\");
 strcat(ptr, minibuf);
 return do_it2(source, fnbuf4);
}

int main (int argc, char **argv)
{
 char *ptr1;
 struct dos_fcb fcb1;
 
 if (argc>3)
 {
  dos_puts(e_args);
  return 1;
 }
 
 if (argc==3)
 {
  strcpy(fnbuf1, argv[1]);
  strcpy(fnbuf2, argv[2]);
  goto args2;
 }
 if (argc==2)
 {
#ifdef HELP
  if (!strcmp(argv[1], "/?"))
  {
   dos_puts(e_help);
   return 1;
  }
#endif
  strcpy(fnbuf1, argv[1]);
  goto args1;
 }
args0:
 dos_puts(e_first);
 dos_getlin(fnbuf1, 128);
 if (!*fnbuf1) goto args0;
args1:
 dos_puts(e_second);
 dos_getlin(fnbuf2, 128);
 if (!*fnbuf2) goto args1;
args2:
 ptr1=basename(fnbuf1);
 dos_parsfnm(ptr1, &fcb1, 0);
 if (!findwild(fcb1))
  do_it1(fnbuf1, fnbuf2);
 else
 {
  struct dos_ffblk ffblk;
  int e;
  
  e=dos_findfirst(fnbuf1, 0, &ffblk);
  if ((!e)&&(dos_errno==0x12))
  {
   dos_puts(fnbuf1);
   dos_puts(e_none);
  }
  while (e)
  {
   strcpy(fnbuf3, fnbuf1);
   strcpy(basename(fnbuf3), ffblk.fnam);
   if (do_it1(fnbuf3, fnbuf2)) break;
   
   dos_findnext(&ffblk);
  }
  if (dos_errno&(dos_errno!=12))
  {
   byte tmp;
   
   tmp=dos_errno;
   dos_puts(fnbuf1);
   dos_puts(e_dash);
   dos_errno=tmp;
   wrerr();
  }
 }

again: 
 dos_puts(e_more);
 dos_getlin(fnbuf1, 128);
 if (!*fnbuf1) goto again;
 if (smash(*fnbuf1)=='Y') goto args0;
 if (smash(*fnbuf1)!='N') goto again;
 return 0;
}
