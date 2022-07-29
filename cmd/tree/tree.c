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
 * Known differences from IBM's version:
 * 
 * 1. We don't take care to make sure files are printed in a sane location
 *    relative to folders.  They just go under the folder.
 * 2. We don't bother to print a header.  If you really want that, do a VOL
 *    on the target drive first.
 */

#include <dos.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODE_F 0x01
#define MODE_A 0x02
unsigned char mode;

#define E_NOERROR 0
#define E_DISKERR 1
#define E_DOSVER  2
#define E_BREAK   3

const char *m_noargs  = "Required parameter missing\r\n",
           *m_xsargs  = "Too many parameters\r\n",
           *m_synerr  = "Syntax error\r\n",
           *m_switch  = "Invalid switch - /",
           *m_drive   = "Invalid drive specification\r\n",
           *m_reaper  = "Insufficient memory\r\n",
#ifdef HELP
           *m_help   =
"Graphically displays the directory structure of a drive or path.\r\n\r\n"
"TREE [drive:][path] [/F] [/A]\r\n\r\n"
"  /F  Displays the names of the files in each directory.\r\n"
"  /A  Uses ASCII instead of extended characters.\r\n";
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
unsigned char barme[255];

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

int origdrv;
char origpwd[65];

char dos_getdrive (void)
{
 char r;
 
 _asm mov ah, 0x19;
 _asm int 0x21;
 _asm mov r, al;
 return r;
}

char dos_setdrive (char d)
{
 char r;

 _asm mov ah, 0x0E;
 _asm mov dl, d;
 _asm int 0x21;
 _asm mov r, al;
 return r;
}

int dos_chdir (const char *f)
{
 _asm mov ah, 0x3B;
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

int dos_getcwd (char d, char *b)
{
 _asm mov ah, 0x47;
 _asm mov dl, d;
 _asm mov si, b;
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

void brktrap (int ignored)
{
 dos_chdir(origpwd);
 dos_setdrive(origdrv);
 exit(E_BREAK);
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

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
}

int depth;

int tree (char *path)
{
 struct dos_ffblk *ffblk;
 int e;
 int count;
 char *lastpwd;
 
 if (!depth)
 {
  origdrv=dos_getdrive();
  if (path[1]==':')
  {
   char c;
  
   c=smash(path[0])-'A';
   dos_setdrive(smash(path[0])-'A');
   if (dos_getdrive()!=c)
   {
    dos_puts(m_drive);
    exit(E_DISKERR);
   }
  }
  
  signal(SIGINT, brktrap);

  *origpwd='\\';
  if (dos_getcwd(0, origpwd+1))
  {
   wrerr();
   dos_setdrive(origdrv);
   return 1;
  }
 }
 
 lastpwd=malloc(65);
 if (!lastpwd)
 {
  dos_puts(m_reaper);
  exit(E_DISKERR);
 }
 *lastpwd='\\';
 dos_getcwd(0,lastpwd+1);
 
 if (dos_chdir(path))
 {
  wrerr();
  free(lastpwd);
  return 1;
 }
 
 ffblk=malloc(sizeof(struct dos_ffblk));
 if (!ffblk)
 {
  dos_puts(m_reaper);
  dos_chdir(origpwd);
  dos_setdrive(origdrv);
  exit(E_DISKERR);
 }
 
 /*
  * First, count the number of directories.
  */
 count=0;
 e=dos_findfirst("*.*", ATTR_DIRECTORY, ffblk);
 if (!e)
 {
  if (dos_errno!=12) wrerr();
  goto cleanup;
 }
 while (e)
 {
  /* Skip . and .. */
  if ((ffblk->attr&ATTR_DIRECTORY)&&(ffblk->fnam[0]!='.')) count++;
  
  e=dos_findnext(ffblk);
 }
 
 /*
  * Now go back and display folders;
  * use an elbow instead of a T-joint for the last one.
  */
 e=dos_findfirst("*.*", ATTR_DIRECTORY, ffblk);
 while (e)
 {
  int t;
  
  if ((ffblk->attr&ATTR_DIRECTORY)&&(ffblk->fnam[0]!='.'))
  {
   count--;
   for (t=0; t<depth; t++)
   {
    if (barme[t]) dos_puts ((mode&MODE_A)?"|   ":"\xB3   "); else dos_puts ("    ");
   }
   if (count) dos_puts ((mode&MODE_A)?"+---":"\xC3\xC4\xC4\xC4"); else dos_puts ((mode&MODE_A)?"\\---":"\xC0\xC4\xC4\xC4");
   dos_puts(ffblk->fnam);
   dos_puts(m_crlf);
   barme[depth]=count;
   depth++;

   /* Due to limits of MS-DOS, this should never happen */
   if (depth>255)
   {
    dos_puts(m_reaper);
    dos_chdir(origpwd);
    dos_setdrive(origdrv);
    exit(E_DISKERR);
   }
   tree(ffblk->fnam);
  }
  e=dos_findnext(ffblk);
 }

 /*
  * If user asked for files too, loop back again and show them.
  */
 if (mode&MODE_F)
 {
  e=dos_findfirst("*.*", 0, ffblk);
  while(e)
  {
   if (!(ffblk->attr&ATTR_DIRECTORY))
   {
    int t;
    
    if (*(ffblk->fnam)=='.') continue;
    for (t=0; t<depth; t++)
    {
     if (barme[t]) dos_puts ((mode&MODE_A)?"|   ":"\xB3   "); else dos_puts ("    ");
    }
    dos_puts("    ");
    dos_puts(ffblk->fnam);
    dos_puts(m_crlf);
   }
   e=dos_findnext(ffblk);
  }
 }

cleanup:
 free(ffblk);
 depth--;
 dos_chdir(lastpwd);
 free(lastpwd);
 
 if (!depth)
  dos_setdrive(origdrv);
 
 return 0;
}

void synerr (void)
{
 dos_puts(m_synerr);
 exit(E_DISKERR);
}

int getswitch (char c)
{
 switch (c)
 {
#ifdef HELP
  case '?':
   dos_puts(m_help);
   return E_DISKERR;
#endif
  case 'F':
   mode |= MODE_F;
   break;
  case 'A':
   mode |= MODE_A;
   break;
  default:
   dos_puts(m_switch);
   dos_putc(c);
   dos_puts(m_crlf);
   exit(E_DISKERR);
 }
 
 return 0;
}

int main (int argc, char **argv)
{
 int e, t, x;
 char *p;
 
 mode=0;
 x=0;
 
 /* Search for switches. */
 for (t=1; t<argc; t++)
 {
  int e;
  
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
 
 depth=0;

 for (t=1; t<argc; t++)
 {
  if (*argv[t])
  {
   if (x)
   {
    dos_puts(m_xsargs);
    return 1;
   }
   dos_puts(argv[t]);
   dos_puts(m_crlf);
   e=tree(argv[t]);
   x++;
  }
 }
 if (!x)
 {
  p=malloc(65);
  if (!p)
  {
   dos_puts(m_reaper);
   return 1;
  }
  dos_getcwd(0, p);
  dos_putc(dos_getdrive()+'A');
  dos_puts(":\\");
  dos_puts(p);
  dos_puts(m_crlf);
  free(p);
  e=tree(".");
 }
 
 return e;
}
