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

/*
 * The parser converts the command line into a path to operate on (arg; may
 * contain wildcards), a flag stating whether to recurse into subdirectories
 * (mode&MODE_S), and two flags stating what modes we want to set or unset
 * (maskon, maskoff).
 * 
 * If maskon=0x00 and maskoff=0xFF, then instead of setting attributes, we
 * want to display them.
 */

unsigned char arg[80];
unsigned char maskon, maskoff;
unsigned char gotarg;

#define MODE_S 0x01
unsigned char mode;

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

const char *m_synerr ="Syntax error\r\n",
           *m_switch ="Invalid switch - /",
           *m_args   ="Too many arguments\r\n",
           *m_ram    ="Insufficient memory\r\n",
           *m_nofiles="No files found\r\n",
           *m_space  ="  ",
           *m_crlf   ="\r\n";

#ifdef HELP
const char *m_help=
 "Displays or changes file attributes.\r\n\r\n"
 "ATTRIB [+R | -R] [+A | -A] [+S | -S] [+H | -H] [[drive:][path]filename] [/S]\r\n\r\n"
 "  +   Sets an attribute.\r\n"
 "  -   Clears an attribute.\r\n"
 "  R   Read-only file attribute.\r\n"
 "  A   Archive file attribute.\r\n"
 "  S   System file attribute.\r\n"
 "  H   Hidden file attribute.\r\n"
 "  /S  Processes files in all directories in the specified path.\r\n";
#endif

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

int getswitch (char a)
{
 switch (a)
 {
#ifdef HELP
  case '?':
   dos_puts(m_help);
   return -1;
#endif
  case 'S':
   mode|=MODE_S;
   return 0;
  default:
   dos_puts(m_switch);
   dos_putc(a);
   dos_puts(m_crlf);
   return -1;
 }
}

int getmode (char *x)
{
 int plusminus;
 
 plusminus=-1;
 
 while (*x)
 {
  unsigned char mask;
  
  switch (smash(*x))
  {
   case '+':
    plusminus=1;
    break;
   case '-':
    plusminus=0;
    break;
   case 'R':
    if (plusminus==-1)
    {
     dos_puts(m_synerr);
     return -1;
    }
    if (plusminus) maskon|=ATTR_READONLY; else maskoff&=~ATTR_READONLY;
    break;
   case 'A':
    if (plusminus==-1)
    {
     dos_puts(m_synerr);
     return -1;
    }
    if (plusminus) maskon|=ATTR_ARCHIVE; else maskoff&=~ATTR_ARCHIVE;
    break;
   case 'S':
    if (plusminus==-1)
    {
     dos_puts(m_synerr);
     return -1;
    }
    if (plusminus) maskon|=ATTR_SYSTEM; else maskoff&=~ATTR_SYSTEM;
    break;
   case 'H':
    if (plusminus==-1)
    {
     dos_puts(m_synerr);
     return -1;
    }
    if (plusminus) maskon|=ATTR_HIDDEN; else maskoff&=~ATTR_HIDDEN;
    break;
   default:
    dos_puts(m_synerr);
    return -1;
  }
  x++;
 }
 
 return 0;
}

void process (char *filename, int a)
{
 int e;
 
 /*
  * If no attributes were specified, show them.
  * Otherwise, turn them on and off as requested.
  * 
  * The MS-DOS version will refuse to set certain attribute combinations
  * without setting others; I believe this is a braindead choice, and prefer
  * to assume the user knows what he's doing.
  */
 if ((!maskon)&&(maskoff==0xFF))
 {
  dos_puts(m_space);
  dos_putc((a&ATTR_ARCHIVE)?'A':' ');
  dos_puts(m_space);
  dos_putc((a&ATTR_SYSTEM)?'S':' ');
  dos_putc((a&ATTR_HIDDEN)?'H':' ');
  dos_putc((a&ATTR_READONLY)?'R':' ');
  dos_puts("     ");
  dos_puts(filename);
  dos_puts(m_crlf);
 }
 else
 {
  a|=maskon;
  a&=maskoff;
  if (dos_setattr(filename,a))
  {
   e=dos_errno;
   dos_puts(filename);
   dos_puts(" - ");
   dos_errno=e;
   wrerr();
  }
 }
}

/*
 * The gories.
 * 
 * Since we recurse, we need to be able to pass a filename, but the other
 * arguments are set at parse time and will not change during execution.
 */
int attrib (char *of)
{
 int e, t;
 char *p;
 char *dirname, *filename, *recurse;
 int searchmask;
 struct dos_ffblk *ffblk;
 
 /*
  * If some sort of mask is being used, and it does not involve hidden or
  * system files, do not look for them.
  * 
  * But if we are displaying attributes, look for them, and show all the files
  * we find.
  */
 searchmask=ATTR_HIDDEN|ATTR_SYSTEM;
 if ((!(maskon&searchmask))&&((maskoff&searchmask)==searchmask)) searchmask=0;
 if ((!maskon)&&(maskoff==0xFF))
  searchmask=ATTR_HIDDEN|ATTR_SYSTEM;

 /*
  * Allocate enough memory to hold variables specific to this instance, that
  * might otherwise get trashed if we did not watch out for reentrancy.
  */
 ffblk=malloc(sizeof(struct dos_ffblk));
 if (!ffblk)
 {
  dos_puts(m_ram);
  return 1;
 }
 
 dirname=malloc(80);
 if (!dirname)
 {
  free(ffblk);
  dos_puts(m_ram);
  return 1;
 }
 
 filename=malloc(80);
 if (!filename)
 {
  free(dirname);
  free(ffblk);
  dos_puts(m_ram);
  return 1;
 }
 
 recurse=malloc(80);
 if (!recurse)
 {
  free(filename);
  free(dirname);
  free(ffblk);
  dos_puts(m_ram);
  return 1;
 }
 
 /*
  * Get the dirname, and tack a \ on the end if need be.
  * If it's just a drive letter, we don't want the trailing \ unless it was
  * specifically asked for (A: and A:\ are two different things in MS-DOS).
  */
 p=strrchr(of, '\\');
 if (p)
 {
  strcpy(dirname, of);
  (strrchr(dirname, '\\'))[1]=0;
  p++;
 }
 else
 {
  if (of[1]==':')
  {
   dirname[0]=of[0];
   dirname[1]=':';
   dirname[2]=0;
   p=of+2;
  }
  else
  {
   dirname[0]='.';
   dirname[1]='\\';
   dirname[2]=0;
   p=of;
  }
 }

 /*
  * Handle the easier case first.
  * (No wildcards)
  */
 if (!(strchr(of, '?')||strchr(of, '*')))
 {
  int a;
  
  a=dos_getattr(of);
  if (!dos_errno)
   process(of, a);
 }
 else
 {
  /*
   * Find all files on which we might want to operate.
   * (See above for how we figure this out.)
   */
  t=dos_findfirst(of,searchmask,ffblk);
  if (!t)
  {
   e=dos_errno;
   dos_puts(of);
   dos_puts(" - ");
   if (e==0x12)
    dos_puts(m_nofiles);
   else
   {
    dos_errno=e;
    wrerr();
   }
  }
 
  /* Iterate over all matching files. */
  while (t)
  {
   unsigned a;
  
   strcpy(filename, dirname);
   strcat(filename, ffblk->fnam);
   a=ffblk->attr;

   process (filename, a);
  
   t=dos_findnext(ffblk);
  }
 }

 /*
  * AFTERWARD, if the user wants us to recurse into folders, do that.
  * Keep in mind that FINDFIRST/FINDNEXT will also find the "." and ".."
  * folders, and we do NOT want to recurse into those!
  */
 if (mode&MODE_S)
 {
  strcpy(recurse, dirname);
  strcat(recurse, "*.*");
  t=dos_findfirst(recurse,ATTR_DIRECTORY,ffblk);
  while (t)
  {
   if (*(ffblk->fnam)!='.')
   {
    if (ffblk->attr&ATTR_DIRECTORY)
    {
     int e;
     
     strcpy(recurse, dirname);
     strcat(recurse, ffblk->fnam);
     strcat(recurse, "\\");
     strcat(recurse, p);
     attrib(recurse);
    }
   }
   t=dos_findnext(ffblk);
  }
 }
 
 /*
  * Free our dynamic allocations and beat a retreat.
  */
 free(recurse);
 free(filename);
 free(dirname);
 free(ffblk);
 
 return 0;
}

int main (int argc, char **argv)
{
 int t;

 maskon=0x00;
 maskoff=0xFF;
 memset(arg,0,80);
 gotarg=0;
 
 strcpy(arg, "*.*");
 
 for (t=1; t<argc; t++)
 {
  char *ptr;
  
  ptr=strchr(argv[t], '/');
  if (ptr)
  {
   *ptr=0;
   ptr++;
   if (getswitch(smash(*ptr))) return 1;
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
  
  if ((*argv[t]=='+')||(*argv[t]=='-'))
  {
   if (getmode(argv[t])) return 1;
   continue;
  }
  
  /* Already got an argument. Die screaming. */
  if (gotarg)
  {
   dos_puts(m_args);
   return 1;
  }
  
  /* Mark argument "got" and save it. */
  gotarg=1;
  strcpy(arg, argv[t]);
 }
 
 return attrib(arg);
}
