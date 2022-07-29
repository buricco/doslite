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

/* Special thanks to Ralf Quint */

/*
 * This program is unfinished.
 * First of all, handling of command parsing and disk spanning have not fully
 * been implemented.
 * Second of all, only the MS-DOS 3.3+ backup format is supported, while the
 * actual MS-DOS/PC DOS version of RESTORE can restore both the 3.3+ version
 * and the older versions.
 * And third and most important, IT IS UNTESTED (and what testing has been
 * performed has rooted up a number of major bugs).
 */

/*
 * 1. BACKUP creates two files for each volume: BACKUP.xxx and CONTROL.xxx,
 *    where xxx is the volume number in decimal (1-255).  These files are
 *    placed in a folder called \BACKUP.
 * 2. Files backed up and restored are necessarily relative to the root.
 * 3. There are a lot of ugly sanity checks here, and the code goes out of its
 *    way not to use stdio.h for anything, in order to save space.  (printf()
 *    is in stdio and has a huge footprint.)
 * 4. Certain files should not be restored.  The list includes DOS kernels and
 *    COMMAND.COM.  (In the original, the kernel files are either those of
 *    MS-DOS or PC DOS, and both are recognized; this version also recognizes
 *    that of FreeDOS.  Additionally, the original recognizes CMD.EXE; we opt
 *    not to bother with that.)
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 64
#endif

#define MAXBUFSIZ 16384

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

#define EXIT_SUCCESS  0
#define EXIT_NOTFOUND 1
#define EXIT_BREAK    3
#define EXIT_OTHER    4

#define SEEK_SET      0
#define SEEK_CUR      1
#define SEEK_END      2

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

word dosver;
word epoch_date, epoch_time;  /* oh dear ghed no, not the Epoch Times!      */

#define MODE_S 0x0001         /* Recurse into subfolders; otherwise only    */
                              /* files in \ are restored.                   */
#define MODE_P 0x0002         /* Prompt before replacing a read-only file,  */
                              /* or replacing a newer file with older.      */
#define MODE_B 0x0004         /* Date stamp must precede epoch_date         */
#define MODE_A 0x0008         /* Date stamp must equal or follow epoch_date */
#define MODE_E 0x0010         /* Time stamp must precede epoch_time         */
#define MODE_L 0x0020         /* Time stamp must equal or follow epoch_time */
#define MODE_M 0x0040         /* Revert only                                */
#define MODE_N 0x0080         /* Add only                                   */
#define MODE_D 0x0100         /* Directory only                             */
word mode;

/* The command line arguments, after they have been parsed. */
char srcmask[80], tgtmask[80];

const char *e_noargs  = "Required parameter missing\r\n",
           *e_args    = "Too many parameters\r\n",
           *e_mix     = "Invalid combination of switches\r\n",
           *e_format  = "Invalid or corrupt control file\r\n",
           *e_backup  = "Could not read backup file\r\n",
           *e_synerr  = "Syntax error\r\n",
           *e_switch  = "Invalid switch - /",
           *e_date    = "Invalid date\r\n",
           *e_time    = "Invalid time\r\n",
           *e_chicken = "Skipping file - ",
           *e_open    = "Could not open ",
           *e_write   = "Could not write to ",
           *e_insert1 = "Insert diskette ",
           *e_insert2 = " in drive ",
           *e_pause   = "Press any key to continue...",
           *e_zottop  = "WARNING: File ",
           *e_rozot   = " is read-only",
           *e_rvzot   = " was changed since it was backed up",
           *e_okzot   = ", replace anyway (Y/N)?",
#ifdef HELP
           *e_help    =
"Restores files that were backed up by using the BACKUP command.\n\n"
"RESTORE drive1: drive2:[path[filename]] [/S] [/P] [/B:date] [/A:date] [/E:time]\n"
"  [/L:time] [/M] [/N] [/D]\n\n"
"  drive1:  Specifies the drive on which the backup files are stored.\n"
"  drive2:[path[filename]]\n"
"           Specifies the file(s) to restore.\n"
"  /S       Restores files in all subdirectories in the path.\n"
"  /P       Prompts before restoring read-only files or files changed since\n"
"           the last backup (if appropriate attributes are set).\n"
"  /B       Restores only files last changed on or before the specified date.\n"
"  /A       Restores only files changed on or after the specified date.\n"
"  /E       Restores only files last changed at or earlier than the specified\n"
"           time.\n"
"  /L       Restores only files changed at or later than the specified time.\n"
"  /M       Restores only files changed since the last backup.\n"
"  /N       Restores only files that no longer exist on the destination disk.\n"
"  /D       Displays files on the backup disk that match specifications.\n",
#endif
           *e_crlf    = "\r\n";

const char *cowardly_lion[] = {
 "IO.SYS",
 "MSDOS.SYS",
 "IBMBIO.COM",
 "IBMDOS.COM",
 "KERNEL.SYS",
 "COMMAND.COM",
 ""
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

void brktrap (int ignored)
{
 exit(EXIT_BREAK);
}

int dos_creat (const char *filename, int mode)
{
 int r;

 _asm mov ah, 0x3C;
 _asm mov cx, mode;
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
 int e;
 byte c;

 e=dos_read (handle, &c, 1);
 if (dos_errno) return -1;
 return c;
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

/*
 * Additional MS-DOS file functions.
 *
 * DX must precede CX in GETMTIME and SETMTIME so that comparison can be done
 * using the standard <=> operators (for /U and /D).
 */
long dos_getmtime (int handle)
{
 long x;
 unsigned short c, d;
 _asm mov ax, 0x5700;
 _asm mov bx, handle;
 _asm int 0x21;
 _asm lahf;
 _asm mov flags, ah;
 if (flags&1)
 {
  _asm mov dos_errno, ax;
  return 0;
 }
 dos_errno=0;
 _asm mov c, cx;
 _asm mov d, dx;
 x=d;
 x<<=16;
 x|=c;
 return x;
}

int dos_setmtime (int handle, long thetime)
{
 unsigned short c, d;

 d=(thetime>>16);
 c=(thetime&0xFFFF);

 _asm mov ax, 0x5701;
 _asm mov bx, handle;
 _asm mov dx, d;
 _asm mov cx, c;
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

int dos_mkdir (const char *filename)
{
 _asm mov ah, 0x39;
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

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
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

/*
 * File header for BACKUP\CONTROL.vol (where vol = disk number, 0-padded).
 * File ends with a NUL.
 */

struct bak_hdr
{
 unsigned char hdrlen;        /* 0x8B */
 unsigned char signature[8];  /* "BACKUP  " */
 unsigned char volume;        /* same as the disk */
 unsigned char reserved[128]; /* zero */
 unsigned char islast;        /* 0xFF=last */
};

struct bak_folder
{
 unsigned char  folderlen;    /* 0x46 */
 unsigned char  path[63];     /* ASCIZ */
 unsigned short entries;      /* next n fileents go here */
 unsigned long  reserved;     /* 0xFFFFFFFF */
};

struct bak_fileent
{
 unsigned char  fileentlen;   /* 0x22 */
 unsigned char  filename[12]; /* ASCIZ */
 unsigned char  mode;         /* 0x01=last part; 0x02=succeeded */
 unsigned long  f_length;     /* filesize in bytes */
 unsigned short chunk;        /* nth piece of the file */
 unsigned long  offset;       /* offset into backup.NNN */
 unsigned long  b_length;     /* amount in backup.NNN */
 unsigned char  attr;         /* file attributes */
 unsigned long  timestamp;
};

unsigned char d_date, d_time;

int getsep (char x)
{
 return ((x=='-')||(x=='/')||(x=='.'));
}

char *getno (char *ptr, word *output)
{
 *output=0;

 while ((*ptr>='0')&&(*ptr<='9'))
 {
  *output *= 10;
  *output += (*ptr&0x0F);
  ptr++;
 }

 return ptr;
}

void exit_date (void)
{
 dos_puts(e_date);
 exit(EXIT_OTHER);
}

void exit_time (void)
{
 dos_puts(e_time);
 exit(EXIT_OTHER);
}

word getyr (word before, word num)
{
 if (num >= 1980)
  num -= 1900;
 if ((num < 80)||(num > 199))
  exit_date();
 return (before&0xFF)|(num<<8);
}

word getmo (word before, word num)
{
 if ((!num)||(num>12))
  exit_date();
 return (before&0xFF1F)|(num<<5);
}

word getdy (word before, word num)
{
 if ((!num)||(num>31))
  exit_date();
 return (before&0xFFE0)|num;
}

char *gettime (char *ptr)
{
 word foo;

 if (*ptr==':') ptr++;

 ptr=getno(ptr, &foo);
 if (*ptr==':') ptr++; else exit_time();
 if (foo>23) exit_time();
 epoch_time=foo<<15;
 ptr=getno(ptr, &foo);
 if (*ptr==':') ptr++; else exit_time();
 if (foo>59) exit_time();
 epoch_time|=foo<<5;
 ptr=getno(ptr, &foo);
 if (*ptr==':') ptr++; else exit_time();
 if (foo>59) exit_time();
 epoch_time|=foo>>1;

 return ptr;
}

char *getdate (char *ptr)
{
 byte country[34];
 word foo;
 byte sanity1, sanity2;

 byte *country_ptr;

 country_ptr=country;

 if (*ptr==':') ptr++;

 /*
  * MS-DOS before 2.11 does not support this API call.
  *
  * If for some reason the call fails, assume the American Standard
  * (insert sound of toilet flushing).
  */
 if (dosver<0x020B)
  country[0]=0;
 else
 {
  _asm mov ax, 0x3800;
  _asm mov dx, country_ptr;
  _asm int 0x21;
  _asm lahf;
  _asm mov flags, ah;
  if (flags&1) country[0]=0;  /* default */
 }

 /*
  * 0 = MDY (as US)
  * 1 = DMY (as Europe)
  * 2 = YMD (as Japan)
  *
  * Save the day and month for a sanity check afterward, since we might not
  * know the month when we get the day, and a range check for 1-31 is not
  * enough to ensure a valid day, because 5 months have fewer than 31 days.
  */
 switch (*country)
 {
  case 1:
   ptr=getno(ptr, &foo);
   sanity2=foo;
   epoch_date=getdy(0, foo);
   if (!getsep(*ptr++)) exit_date();
   ptr=getno(ptr, &foo);
   sanity1=foo;
   epoch_date=getmo(epoch_date, foo);
   if (!getsep(*ptr++)) exit_date();
   ptr=getno(ptr, &foo);
   epoch_date=getyr(epoch_date, foo);
   break;
  case 2:
   ptr=getno(ptr, &foo);
   epoch_date=getyr(0, foo);
   if (!getsep(*ptr++)) exit_date();
   ptr=getno(ptr, &foo);
   sanity1=foo;
   epoch_date=getmo(epoch_date, foo);
   if (!getsep(*ptr++)) exit_date();
   ptr=getno(ptr, &foo);
   sanity2=foo;
   epoch_date=getdy(epoch_date, foo);
   break;
  default:
   ptr=getno(ptr, &foo);
   sanity1=foo;
   epoch_date=getmo(0, foo);
   if (!getsep(*ptr++)) exit_date();
   ptr=getno(ptr, &foo);
   sanity2=foo;
   epoch_date=getdy(epoch_date, foo);
   if (!getsep(*ptr++)) exit_date();
   ptr=getno(ptr, &foo);
   epoch_date=getyr(epoch_date, foo);
 }
 switch (sanity1)
 {
  /* "30 days hath September..." */
  case 4:
  case 6:
  case 9:
  case 11:
   if (sanity2>30)
    exit_date();
   break;
  case 2:
   if (sanity2>29)
    exit_date();
   if ((!(epoch_date&0x0300))&(sanity2==29))  /* Oversimplification */
    exit_date();
 }

 epoch_date=foo;
 return ptr;
}

void genext (byte vol, char *str)
{
 char *p=str+3;

 *p=0;
 str[0]=str[1]=str[2]='0';
 str[3]=0;
 while (vol)
 {
  *(--p)='0'+(vol%10);
  vol/=10;
 }
}

/*
 * Return 1 if tgt matches src, accounting for wildcards.
 */
int fnmatch (char *src, char *tgt)
{
 struct dos_fcb fcb1, fcb2;
 char f_unbuild1[PATH_MAX],
      f_unbuild2[PATH_MAX];
 char *fnptr1, *fnptr2;
 byte e;

 strcpy(f_unbuild1, src);
 strcpy(f_unbuild2, tgt);
 fnptr1=strrchr(f_unbuild1, '\\');
 fnptr2=strrchr(f_unbuild2, '\\');
 if (fnptr1)
 {
  *(fnptr1++)=0;
 }
 else
 {
  *f_unbuild1=0;
  fnptr1=src;
 }
 if (fnptr2)
 {
  *(fnptr2++)=0;
 }
 else
 {
  *f_unbuild2=0;
  fnptr2=tgt;
 }

 /*
  * If no path is specified, any path is acceptable.
  */
 if (f_unbuild2)
  if (strcmp(f_unbuild1, f_unbuild2)) return 0;

 dos_parsfnm(fnptr1, &fcb1, 0);
 e=dos_parsfnm(fnptr2, &fcb2, 0);
 if (e==1)
 {
  byte t, *f, *g;
  f=(byte *)(fcb1.filename);
  g=(byte *)(fcb2.filename);
  for (t=0; t<11; t++) if (g[t]=='?') g[t]=f[t];
  for (t=0; t<11; t++) if (f[t]!=g[t]) return 0;
 }
 else
 {
  return !strcmp(fnptr1, fnptr2);
 }
 return 1;
}

int restore (char *path, char *target)
{
 int backup, ctrl;
 char f_backup[PATH_MAX], f_ctrl[PATH_MAX],
      f_lastpath[PATH_MAX], f_build[PATH_MAX],
      f_fullpath[PATH_MAX];
 char ext[4];
 int c;
 int d;
 int n;
 int foldercount;
 struct bak_hdr bak_hdr;
 struct bak_folder bak_folder;
 struct bak_fileent bak_fileent;
 char targdrv[3];
 char fnam[13];
 word t_date, t_time;

 d=n=0;

 *f_lastpath=0;

 strcpy(targdrv+1, ":");
 *targdrv=*target;

 while (!n)
 {
  d++;

  genext(d, ext);
  strcpy(f_backup, path);
  strcpy(f_ctrl, path);
  strcat(f_backup, "\\BACKUP\\BACKUP.");
  strcat(f_ctrl, "\\BACKUP\\CONTROL.");
  strcat(f_backup, ext);
  strcat(f_ctrl, ext);

  while (1)
  {
   ctrl=dos_open(f_ctrl, 0);
   if (ctrl)
   {
    break;
   }
   else
   {
    dos_puts(e_insert1);
    dos_puts(ext);
    dos_puts(e_insert2);
    dos_putc(*path);
    dos_putc(':');
    dos_puts(e_crlf);
    dos_puts(e_pause);
    dos_getche();
    dos_puts(e_crlf);
    continue;
   }
  }

  dos_read(ctrl, &bak_hdr, sizeof(bak_hdr));
  if (bak_hdr.hdrlen!=0x8B)
  {
   dos_close(ctrl);
   dos_puts(e_format);
   return -1;
  }
  n=bak_hdr.islast;

  while (1)
  {
   c=dos_getc(ctrl);
   if (c<0)
   {
    dos_close(ctrl);
    break;
   }
   dos_lseek(ctrl,-1,SEEK_CUR);
   if (c==0x46)
   {
    dos_read(ctrl, &bak_folder, sizeof(bak_folder));
    foldercount=bak_folder.entries;
    strcpy(f_lastpath, bak_folder.path);

    strcpy(f_fullpath, targdrv);
    strcat(f_fullpath, "\\");
    strcpy(f_fullpath, f_lastpath);
    dos_mkdir(f_fullpath);
   }
   else if (c==0x22)
   {
    int iter, hand;
    dword l, r;

    dos_read(ctrl, &bak_fileent, sizeof(bak_fileent));

    fnam[12]=0;
    strncpy(fnam, bak_fileent.filename, 12);
    if (*f_lastpath)
    {
     strcpy(f_build, f_lastpath);
    }
    else
    {
     f_build[0]=target[0];
     f_build[1]=':';
     f_build[2]=0;
    }
    strcpy(f_build, "\\");
    strcat(f_build, fnam);
    if (foldercount)
    {
     foldercount--;
     if (!foldercount)
      *f_lastpath=0;
     if ((*f_lastpath)&&(!(mode&MODE_S)))
     {
      continue; /* Do not recurse */
     }
    }

    /*
     * If a file mask was specified, match against it.
     */
    if (target[2])
     if (!fnmatch(f_build, target+2)) continue;

    /*
     * If requested, match against a specified date and time.
     */
    t_date=bak_fileent.timestamp>>16;
    t_time=bak_fileent.timestamp&0xFFFF;

    if ((mode&MODE_E)&&(t_time>epoch_time))  continue;
    if ((mode&MODE_L)&&(t_time<=epoch_time)) continue;
    if ((mode&MODE_B)&&(t_date>epoch_date))  continue;
    if ((mode&MODE_A)&&(t_date<=epoch_date)) continue;

    if (mode&MODE_D) /* Only do directory */
    {
     dos_puts (f_build);
     dos_puts (e_crlf);
    }
    else
    {
     int attr;
     
     /*
      * Certain files must not be replaced.
      * The list here has been changed; CMD.EXE (an OS/2 file) has been
      * replaced with KERNEL.SYS (a FreeDOS file).
      */
     for (iter=0; *cowardly_lion[iter]; iter++)
     {
      if (!strcmp(f_build, cowardly_lion[iter]))
      {
       dos_puts(e_chicken);
       dos_puts(f_build);
       dos_puts(e_crlf);
       continue;
      }
     }

     strcpy(f_fullpath, targdrv);
     strcat(f_fullpath, "\\");
     strcat(f_fullpath, f_build);
     backup=dos_open(f_backup, 0);
     if (!backup)
     {
      dos_close(ctrl);
      dos_puts(e_backup);
      return -1;
     }
     iter=0;
     l=bak_fileent.b_length;
     dos_lseek(backup, bak_fileent.offset, SEEK_SET);

     attr=dos_getattr(f_fullpath);
     if (!dos_errno)
     {
      dword t;

      hand=dos_open(f_fullpath, 0);
      if (hand)
      {
       dos_puts(e_open);
       dos_puts(f_fullpath);
       dos_puts(e_crlf);
       dos_close(backup);
       dos_close(ctrl);
       return -1;
      }
      t=dos_getmtime(hand);
      dos_close(hand);
      if (mode&MODE_N) continue;
      if ((mode&MODE_M)&&(t<=bak_fileent.timestamp)) continue;
      if (mode&MODE_P) /* read-only or newer, prompt for it */
      {
       if (attr&ATTR_READONLY)
       {
        byte yn;

        while (1)
        {
         dos_puts(e_zottop);
         dos_puts(f_fullpath);
         dos_puts(e_rozot);
         dos_puts(e_okzot);
         yn=smash(dos_getche());
         dos_puts(e_crlf);
         if ((yn=='Y')||(yn=='N')) break;
        }
        if (yn=='N') continue;
       }
       if (t>bak_fileent.timestamp)
       {
        byte yn;

        while (1)
        {
         dos_puts(e_zottop);
         dos_puts(f_fullpath);
         dos_puts(e_rvzot);
         dos_puts(e_okzot);
         yn=smash(dos_getche());
         dos_puts(e_crlf);
         if ((yn=='Y')||(yn=='N')) break;
        }
        if (yn=='N') continue;
       }
      }
     }

     dos_setattr(f_fullpath, 0);
     hand=dos_creat(f_fullpath, 0);
     if (!hand)
     {
      dos_puts(e_open);
      dos_puts(f_fullpath);
      dos_puts(e_crlf);
      dos_close(backup);
      dos_close(ctrl);
      return -1;
     }
     while (l)
     {
      byte buf[MAXBUFSIZ];

      r=(l>MAXBUFSIZ)?l:MAXBUFSIZ;
      l-=dos_read(backup, buf, r);
      if (dos_write(hand, buf, r)!=r)
      {
       dos_puts(e_write);
       dos_puts(f_fullpath);
       dos_puts(e_crlf);
       dos_close(hand);
       dos_close(backup);
       dos_close(ctrl);
       return -1;
      }
     }
     dos_setmtime(hand, bak_fileent.timestamp);
     dos_close(hand);
     dos_setattr(f_fullpath, bak_fileent.attr);
     dos_close(backup);
    }
   }
   else
   {
    dos_close(ctrl);
    dos_puts(e_format);
    return -1;
   }
  }
 }

 return 0;
}

int getswitch (char a)
{
 switch (a)
 {
#ifdef HELP
  case '?':
   dos_puts(e_help);
   exit(1);
#endif
  case 'D':
   mode|=MODE_D;
   break;
  case 'M':
   mode|=MODE_M;
   break;
  case 'N':
   mode|=MODE_N;
   break;
  case 'P':
   mode|=MODE_P;
   break;
  case 'S':
   mode|=MODE_S;
   break;
  default:
   dos_puts(e_switch);
   dos_putc(a);
   dos_puts(e_crlf);
   return -1;
 }

 return 0;
}

int main (int argc, char **argv)
{
 char *targ;
 char *ptr;
 int t;

 /* How we handle dates depends on if we're running DOS 2.11 or later */
 _asm mov ah, 0x30;
 _asm int 0x21;
 _asm xchg ah, al;
 _asm mov dosver, ax;

 /* Set defaults. */
 targ=srcmask;
 mode=0;

 /* Scream if no argument specified. */
 if (argc==1)
 {
  dos_puts(e_noargs);
  return EXIT_OTHER;
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

   if (smash(*ptr)=='A')
   {
    mode|=MODE_A;
    ptr=getdate(ptr+1)-1;
   }
   if (smash(*ptr)=='B')
   {
    mode|=MODE_B;
    ptr=getdate(ptr+1)-1;
   }
   if (smash(*ptr)=='E')
   {
    mode|=MODE_E;
    ptr=gettime(ptr+1)-1;
   }
   if (smash(*ptr)=='L')
   {
    mode|=MODE_L;
    ptr=gettime(ptr+1)-1;
   }

   else if (getswitch(smash(*ptr)))
    return EXIT_OTHER;
   ptr++;
   if (!*ptr) continue;
   while (*ptr)
   {
    if (*ptr!='/')
    {
     dos_puts(e_synerr);
     return EXIT_OTHER;
    }
    ptr++;

    if (smash(*ptr)=='A')
    {
     mode|=MODE_A;
     ptr=getdate(ptr+1)-1;
    }
    if (smash(*ptr)=='B')
    {
     mode|=MODE_B;
     ptr=getdate(ptr+1)-1;
    }
    if (smash(*ptr)=='E')
    {
     mode|=MODE_E;
     ptr=gettime(ptr+1)-1;
    }
    if (smash(*ptr)=='L')
    {
     mode|=MODE_L;
     ptr=gettime(ptr+1)-1;
    }
    if (getswitch(smash(*ptr))) return EXIT_OTHER;
    ptr++;
   }
  }
  
  /* If the beginning of the argument was /, it is now a NUL terminator. */
  if (!*argv[t]) continue;

  /*
   * Weed out incompatible switches:
   * /A+/B = contradiction
   * /E+/L = contradiction
   * /M+/N = contradiction
   * /D+/P = makes no sense
   */
  if ((mode&(MODE_A|MODE_B))==(MODE_A|MODE_B))
  {
   dos_puts(e_mix);
   return EXIT_OTHER;
  }
  if ((mode&(MODE_E|MODE_L))==(MODE_E|MODE_L))
  {
   dos_puts(e_mix);
   return EXIT_OTHER;
  }
  if ((mode&(MODE_M|MODE_N))==(MODE_M|MODE_N))
  {
   dos_puts(e_mix);
   return EXIT_OTHER;
  }
  if ((mode&(MODE_D|MODE_P))==(MODE_D|MODE_P))
  {
   dos_puts(e_mix);
   return EXIT_OTHER;
  }

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
   dos_puts(e_args);
   return EXIT_OTHER;
  }
 }

 /* If we did not get a source argument, then die screaming. */
 if (targ==srcmask)
 {
  dos_puts(e_noargs);
  return EXIT_OTHER;
 }

 if ((strcmp(srcmask+1, ":"))||(tgtmask[1]!=':'))
 {
  dos_puts(e_synerr);
  return EXIT_OTHER;
 }

 *srcmask=smash(*srcmask);
 *tgtmask=smash(*tgtmask);
 if ((*srcmask<'A')||(*srcmask>'Z')||(*tgtmask<'A')||(*tgtmask>'Z'))
 {
  dos_puts(e_synerr);
  return EXIT_OTHER;
 }

 signal(SIGINT, brktrap);
 return restore(srcmask, tgtmask);
}
