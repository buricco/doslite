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

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODE_Y 0x01
unsigned char mode;

const char *m_noargs  = "Required parameter missing\r\n",
           *m_synerr  = "Syntax error\r\n",
           *m_switch  = "Invalid switch - /",
           *m_ram     = "Insufficient memory\r\n",
           *m_del1    = "Delete file \"",
           *m_del2    = "\" (Y/N)? ",
           *m_blast1  = "Delete directory \"",
           *m_blast2  = "\" and all its subdirectories (Y/N)? ",
           *m_zotting = "Deleting: ",
           *m_ellipse = "...",
           *m_ok      = "OK\r\n",
#ifdef HELP
           *m_help   =
"Deletes a directory and all the subdirectories and files in it.\r\n\r\n"
"DELTREE [/Y] [drive:]path [[drive:]path[...]]\r\n\r\n"
"  /Y             Suppresses prompting to confirm you want to delete\r\n"
"                 the subdirectory.\r\n"
"  [drive:]path   Specifies the name of the directory you want to delete.\r\n\r\n"
"WARNING: Use DELTREE with extreme care!  Every file and subdirectory within the\r\n"
"         specified directory will be deleted.\r\n";
#endif
           *m_crlf   = "\r\n";

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

unsigned char flags;

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

/* Dependency of dos_puts */
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

int dos_setattr (const char *filename, int attr)
{
 _asm mov ax, 0x4301;
 _asm mov cx, attr;
 _asm mov dx, filename;
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

int dos_remove (const char *f)
{
 _asm mov ah, 0x41;
 _asm mov dx, f;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 1;
 }
 dos_errno=0;
 return 0;
}

int dos_rmdir (const char *f)
{
 _asm mov ah, 0x3A;
 _asm mov dx, f;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 1;
 }
 dos_errno=0;
 return 0;
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

char dos_getche (void)
{
 char b;
 
 _asm mov ah, 0x01;
 _asm int 0x21;
 _asm mov b, al;
 return b;
}

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
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

void blast (char *path)
{
 char *filename, *buildname;
 struct dos_ffblk *ffblk;
 char *p;
 int t;

 /*
  * Allocate enough memory to hold variables specific to this instance, that
  * might otherwise get trashed if we did not watch out for reentrancy.
  */
 ffblk=malloc(sizeof(struct dos_ffblk));
 if (!ffblk)
 {
  dos_puts(m_ram);
  exit(2);
 }
 
 filename=malloc(80);
 if (!filename)
 {
  free(ffblk);
  dos_puts(m_ram);
  exit(2);
 }
 
 buildname=malloc(80);
 if (!buildname)
 {
  free(filename);
  free(ffblk);
  dos_puts(m_ram);
  exit(2);
 }

 strcpy(buildname, path);
 strcat(buildname, "\\*.*");

 /* Files first */
 t=dos_findfirst(buildname,ATTR_HIDDEN|ATTR_SYSTEM,ffblk);
 while (t)
 {
  strcpy(filename, path);
  strcpy(filename, "\\");
  strcat(filename, ffblk->fnam);
  dos_setattr(filename, 0);
  dos_remove(filename);
  t=dos_findnext(ffblk);
 }

 /* Then directories */
 t=dos_findfirst(buildname,ATTR_HIDDEN|ATTR_SYSTEM|ATTR_DIRECTORY,ffblk);
 while (t)
 {
  if (*(ffblk->fnam)!='.')
  {
   strcpy(filename, path);
   strcpy(filename, "\\");
   strcat(filename, ffblk->fnam);
   dos_setattr(filename, ATTR_DIRECTORY);
   dos_puts(m_zotting);
   dos_puts(filename);
   dos_puts(m_ellipse);
   blast(filename);
  }
  t=dos_findnext(ffblk);
 }
 
 free(buildname);
 free(filename);
 free(ffblk);
 
 dos_rmdir(path);
}

void deltree (char *path)
{
 int e;
 
 /* Directory or file? */
 e=dos_getattr(path);
 if (dos_errno)
 {
  wrerr();
  exit(2);
 }
 
 if (e&ATTR_DIRECTORY)
 {
  /* Directory. */
  if (!(mode&MODE_Y))
  {
   while (1)
   {
    char c;
    
    dos_puts(m_blast1);
    dos_puts(path);
    dos_puts(m_blast2);
    c=smash(dos_getche());
    dos_puts(m_crlf);
    if (c=='N') return;
    if (c=='Y') break;
   }
  }
     
  dos_puts(m_zotting);
  dos_puts(path);
  dos_puts(m_ellipse);
  blast(path);
  return;
 }

 /* File. */
 if (!(mode&MODE_Y))
 {
  while (1)
  {
   char c;
   
   dos_puts(m_del1);
   dos_puts(path);
   dos_puts(m_del2);
   c=smash(dos_getche());
   dos_puts(m_crlf);
   if (c=='N') return;
   if (c=='Y') break;
  }
 }
     
 dos_puts(m_zotting);
 dos_puts(path);
 dos_puts(m_ellipse);
 if (dos_remove(path)) wrerr(); else dos_puts(m_ok);
}

void synerr (void)
{
 dos_puts(m_synerr);
 exit(1);
}

int getswitch (char c)
{
 switch (c)
 {
#ifdef HELP
  case '?':
   dos_puts(m_help);
   return -1;
#endif
  case 'Y':
   mode |= MODE_Y;
   break;
  default:
   dos_puts(m_switch);
   dos_putc(c);
   dos_puts(m_crlf);
   exit(1);
 }
 
 return 0;
}

int main (int argc, char **argv)
{
 int t, x;
 mode=0;
 
 /*
  * Search for switches.
  * (Only /Y is valid, unless help is compiled in.)
  */
 for (t=1; t<argc; t++)
 {
  int e;
  char *p;
  
  p=strchr(argv[t], '/');
  while (p)
  {
   *p=0;
   if (!p[1]) synerr();
   e=getswitch(smash(p[1]));
   if (e) return e;
   p+=2;
   if (!*p) break;
   if (*p=='/')
   {
    p++;
    continue;
   }
   synerr();
  }
 }
 
 x=0;
 
 for (t=1; t<argc; t++)
 {
  if (*argv[t])
  {
   x++;
   deltree(argv[t]);
  }
 }
 
 if (!x)
 {
  dos_puts(m_noargs);
  return 1;
 }
 
 return 0;
}
