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
 * If you set the stack size too small, the program will crash with a stack
 * overflow, but if you set it too large, it will crash with out of memory.
 * 
 * Current advice: use wcl -k12288 (12K stack).
 */

/*
 * A little historical note about the /U and /D switches...
 * 
 * For the DOS 3.20 cycle, two different versions of the REPLACE command were
 * written, which are cosmetically similar, but do not appear to be based on
 * the same code (I did not reverse-engineer the code to see).
 * 
 * The Microsoft version featured an additional switch over the IBM version,
 * /D, which replaced files which were newer than the source.
 * 
 * In the DOS 3.3 cycle, Microsoft did not actually produce the new version of
 * MS-DOS - IBM did - so MS-DOS 3.3 got IBM's crippled version of the utility.
 * In version 4.0, also engineered by IBM rather than Microsoft, they opted to
 * add a switch to do the exact opposite, /U - to replace files which were
 * older than the source.  This version was maintained in all later versions
 * of MS-DOS and PC DOS with only minor adjustments.
 * 
 * In this version, I have opted to add both the /U and /D switches, as I find
 * that they both have their uses.
 */

/*
 * Return codes:
 * 
 * 0 - Success
 * 1 - Incorrect DOS version
 * 2 - No source files found
 * 11 - Syntax error
 * Other - DOS error code
 * 
 * A thing to note: All messages should use CP/M newlines (i.e., \r\n not \n),
 * because we are skipping most of the libc, including the code that converts 
 * Unix newlines to CP/M format as MS-DOS requires.
 */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Mask for switches.
 * 
 * This allows us to compress up to 8 (char), 16 (short), 32 (long) switches 
 * into a simple bitmap, instead of using separate variables for each and
 * wasting a ton of memory.
 */
#define MODE_A 0x01
#define MODE_P 0x02
#define MODE_R 0x04
#define MODE_S 0x08
#define MODE_W 0x10
#define MODE_U 0x20
#define MODE_D 0x40
unsigned char mode;

/*
 * It is clearer to use a macro every time we return this code, than to write
 * down a magic number for the same purpose.
 */
#define EXIT_OLDDOS 1
#define EXIT_NOFILE 2
#define EXIT_SYNERR 11

/*
 * Size of our copy buffer.
 * Default, 16K, is reasonable.
 * Limit is 65535.
 */
#define IOBUFSIZ 0x4000

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

/* Hold an error number for exit. */
int retval;

/* Number of files successfully copied. */
long tally;

/* Keep all messages here for organization. */
const char *m_olddos  = "Incorrect DOS version\r\n",
           *m_noargs  = "Required parameter missing\r\n",
           *m_args    = "Too many parameters\r\n",
           *m_synerr  = "Syntax error\r\n",
           *m_clash   = "Invalid combination of parameters\r\n",
           *m_switch  = "Invalid switch - /",
           *m_press   = "Press any key to start ",
           *m_add     = "add",
           *m_rep     = "replac",
           *m_ing     = "ing files...",
           *m_yn      = " (Y/N)? ",
           *m_files   = " file(s) ",
           *m_ed      = "ed.\r\n",
           *m_nofiles = "No files found\r\n",
           *m_mem     = "Insufficient memory\r\n",
           *m_self    = "Cannot copy file to itself\r\n",
           *crlf      = "\r\n";

#ifdef HELP
const char *m_help=
 "Replaces files.\r\n\r\n"
 "REPLACE [drive1:][path1]filename [drive2:][path2] [/A] [/P] [/R] [/W]\r\n"
 "REPLACE [drive1:][path1]filename [drive2:][path2] [/P] [/R] [/S] [/W] [/U | /D]\r\n\r\n"
 "  [drive1:][path1]filename Specifies the source file or files.\r\n"
 "  [drive2:][path2]         Specifies the directory where files are to be\r\n"
 "                           replaced.\r\n"
 "  /A                       Adds new files to destination directory. Cannot\r\n"
 "                           use with /S, /U or /D switches.\r\n"
 "  /P                       Prompts for confirmation before replacing a file\r\n"
 "                           or adding a source file.\r\n"
 "  /R                       Replaces read-only files as well as unprotected\r\n"
 "                           files.\r\n"
 "  /S                       Replaces files in all subdirectories of the\r\n"
 "                           destination directory. Cannot use with the /A\r\n"
 "                           switch.\r\n"
 "  /W                       Waits for you to insert a disk before beginning.\r\n"
 "  /U                       Replaces (updates) only files that are older than\r\n"
 "                           source files. Cannot use with the /A or /D switch.\r\n"
 "  /D                       Replaces only files that are newer than source\r\n"
 "                           files. Cannot use with the /A or /U switch.\r\n";
#endif

/* The command line arguments, after they have been parsed. */
char srcmask[80], tgtmask[80];

/* Generated paths. */
char srcgen[80], tgtgen[80];

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

/*
 * Copy a file.
 * You must supply complete filenames, but paths are optional.
 */
int daftcopy (char *source, char *target)
{
 unsigned char buffer[IOBUFSIZ], *bufptr;
 unsigned char e;
 int r;
 int file1, file2;
 
 bufptr=buffer;
 
 _asm mov ax, 0x3D00;         /* OPEN */
 _asm mov dx, source;
 _asm int 0x21;
 _asm mov file1, ax;
 _asm lahf;                   /* Return error if carry set */
 _asm mov e, ah;
 if (e&1) goto err;
 
 _asm mov ah, 0x3C;           /* CREAT */
 _asm xor cx, cx;
 _asm mov dx, target;
 _asm int 0x21;
 _asm mov file2, ax;
 _asm lahf;                   /* Return error if carry set */
 _asm mov e, ah;
 if (e&1) goto err;
 while (1)
 {
  _asm mov cx, IOBUFSIZ;
  _asm mov dx, bufptr;
  _asm mov bx, file1;
  _asm mov ah, 0x3F;          /* READ */
  _asm int 0x21;
  _asm mov r, ax;
  _asm lahf;                  /* Return error if carry set */
  _asm mov e, ah;
  if (e&1) goto err;
  if (!r) break;              /* Something more to write? */
  _asm mov cx, r;             /* Number of bytes read by previous call */
  _asm mov ah, 0x40;          /* WRITE */
  _asm mov bx, file2;
  _asm mov dx, bufptr;
  _asm int 0x21;
  _asm lahf;
  _asm mov e, ah;
  if (e&1) goto err;
 }
 _asm mov ax, 0x5700;         /* GETFTIME */
 _asm mov bx, file1;
 _asm int 0x21;
 _asm mov ax, 0x5701;         /* SETFTIME */
 _asm mov bx, file2;
 _asm int 0x21;
 _asm mov ax, 0x4300;         /* GETATTR */
 _asm mov dx, source;
 _asm int 0x21;
 _asm mov ax, 0x4301;         /* SETATTR */
 _asm mov dx, target;
 _asm int 0x21;
 return 0;
err:
 _asm mov e, al;
 return e;
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

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
}

/* Pause prompt for /W switch. */
void pause (void)
{
 /* Reset all disks. */
 _asm mov ah, 0x0D;
 _asm int 0x21;
 
 /* Prompt and wait for keypress. */
 dos_puts (m_press);
 dos_puts((mode&MODE_A)?m_add:m_rep);
 dos_puts(m_ing);
 dos_getche();
 dos_puts(crlf);
}

/*
 * Generic error printer. (Call as soon as possible after the error.)
 * Also sets retval to the current error value, so we have it at quit time.
 */
void wrerr (void)
{
 int p;
 
 retval=dos_errno;
 for (p=0; errtab[p].err; p++)
 {
  if (errtab[p].err==retval) break;
 }
 dos_puts(errtab[p].msg);
 dos_puts(crlf);
}

/* Display the number of files copied. */
void summarize (void)
{
 char buf[12];
 char *ptr;
 unsigned long x;
 
 /* Set our counters, then flush the buffer. */
 x=tally;
 memset(buf,0,12);
 ptr=buf+10;
 
 /* Convert the number to a string, digit by digit. */
 buf[10]='0';
 while (x)
 {
  char digit;
  
  digit=x%10;
  *(ptr--)=digit|'0';
  x/=10;
  if (ptr<buf)
   break; /* overflow */
 }
 if (tally) ptr++;
 
 /* Write our message. */
 dos_puts(crlf);
 dos_puts(ptr);
 dos_puts(m_files);
 dos_puts((mode&MODE_A)?m_add:m_rep);
 dos_puts(m_ed);
}

/*
 * Use TRUENAME to make sure we aren't copying a file over itself.
 * (This requires MS-DOS 3 or later)
 */
int sanitycheck (char *source, char *target)
{
 unsigned char e;
 char buf1[128], buf2[128];
 char *ptr1, *ptr2;
 
 ptr1=buf1;
 ptr2=buf2;
 
 _asm mov si, source;
 _asm mov di, ptr1;
 _asm mov ah, 0x60;           /* TRUENAME */
 _asm int 0x21;
 _asm lahf;
 _asm mov e, ah;
 if (e&1) goto err;
 _asm mov si, target;
 _asm mov di, ptr2;
 _asm mov ah, 0x60;           /* TRUENAME */
 _asm int 0x21;
 _asm lahf;
 _asm mov e, ah;
 if (e&1) goto err;
 return (strcmp(ptr1, ptr2))?0:1;
err:
 _asm mov e, al;
 return e;
}

int confirm (char *source, char *target)
{
 int k;
 int a;
 int p;

 /*
  * If /A is not specified, make sure the target file exists, but do not 
  * return an error if it does not.
  * 
  * If /A is specified, make sure the target file does NOT exist, but do not
  * return an error if it does.
  * 
  * In either case, a fail should simply return 0.
  */
 a=dos_getattr(target);
 dos_errno &= 0xFF;
 if (mode&MODE_A)
 {
  if (dos_errno!=2)
   return 0;
 }
 else
 {
  if (dos_errno==2)
   return 0;
 }
 
 /*
  * Check, if requested, the date stamps of our files.
  * Since this is always in REPLACE mode, being unable to get a stamp off the
  * target file should simply register as "skip".
  */
 if (mode&(MODE_U|MODE_D))
 {
  unsigned long t1, t2;
  int c;
  
  c=dos_open(target, 0);
  if (!c)
   return 0;
  
  t1=dos_getmtime(c);
  dos_close(c);

  c=dos_open(target, 0);
  if (!c)
   return 0;
  
  t2=dos_getmtime(c);
  dos_close(c);
  
  if (mode&MODE_U)
   c=(t2<t1);
  else
   c=(t2>t1);
  
  if (!c) return 0;
 }

 /*
  * Display target filename.
  * If /P is requested, also ask whether to go through with the copy.
  */
 do
 {
  dos_puts(target);
  if (mode&MODE_P)
  {
   dos_puts(m_yn);
   k=smash(dos_getche());
   dos_puts(crlf);
   if (k=='N')
    return 0;
   if (k=='Y')
    break;
  }
  else
  {
   dos_puts(crlf);
   break;
  }
 } while (1);
  
 /*
  * If /R is specified, take the attributes we got, mask off READONLY, and
  * set them back, meaning CREAT will work as intended.
  * 
  * If not, then when we get to CREAT, a read-only file will cause us to burn,
  * which is actually the correct behavior.
  */
 if (mode&MODE_R)
  dos_setattr(target, a&(~ATTR_READONLY));
 
 return 1;
}

int replace (char *source, char *target)
{
 int e, t;
 char *p;
 char *dirname, *filename, *recurse;
 struct dos_ffblk *ffblk;
 
 
 /*
  * Allocate enough memory to hold variables specific to this instance, that
  * might otherwise get trashed if we did not watch out for reentrancy.
  */
 ffblk=malloc(sizeof(struct dos_ffblk));
 if (!ffblk)
 {
  dos_puts(m_mem);
  return 1;
 }
 
 dirname=malloc(80);
 if (!dirname)
 {
  free(ffblk);
  dos_puts(m_mem);
  return 1;
 }
 
 filename=malloc(80);
 if (!filename)
 {
  free(dirname);
  free(ffblk);
  dos_puts(m_mem);
  return 1;
 }
 
 recurse=malloc(80);
 if (!recurse)
 {
  free(filename);
  free(dirname);
  free(ffblk);
  dos_puts(m_mem);
  return 1;
 }

 /*
  * Get the dirname, and tack a \ on the end if need be.
  * If it's just a drive letter, we don't want the trailing \ unless it was
  * specifically asked for (A: and A:\ are two different things in MS-DOS).
  */
 p=strrchr(source, '\\');
 if (p)
 {
  char *q;
  
  strcpy(dirname, source);
  q=strrchr(dirname, '\\');
  if (q) /* safety valve */
  {
   q[1]=0;
   p++;
  }
 }
 else
 {
  if (source[1]==':')
  {
   dirname[0]=source[0];
   dirname[1]=':';
   dirname[2]=0;
   p=&(source[2]);
  }
  else
  {
   strcpy(dirname, ".\\");
   p=source;
  }
 }
 
 /*
  * Find all files on which we might want to operate.
  * (See above for how we figure this out.)
  */
 t=dos_findfirst(source,0,ffblk);
 if (!t)
 {
  e=dos_errno;
  dos_puts(source);
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
  strcpy(filename, dirname);
  strcat(filename, ffblk->fnam);
  
  strcpy(recurse, target);
  if ((recurse[strlen(recurse)-1]!=':')&&(recurse[strlen(recurse)-1]!='\\'))
   strcat(recurse, "\\");
  strcat(recurse, ffblk->fnam);
  
  e=sanitycheck(filename, recurse);
  if (e==1)
  {
   dos_puts(m_self);

   free(recurse);
   free(filename);
   free(dirname);
   free(ffblk);
   return 0; /* checked with PC DOS 7 */
  }
  else if (e)
  {
   dos_errno=e;
   wrerr();
  }
  else
  {
   if (confirm(filename, recurse))
   {
    e=daftcopy(filename, recurse);
    if (e)
    {
     dos_errno=e;
     wrerr();
    }
    else
     tally++;
   }
  }
  
  t=dos_findnext(ffblk);
 }
 
 /*
  * AFTERWARD, if the user wants us to recurse into folders, do that.
  * Keep in mind that FINDFIRST/FINDNEXT will also find the "." and ".."
  * folders, and we do NOT want to recurse into those!
  */
 if (mode&MODE_S)
 {
  strcpy(recurse, target);
  if ((recurse[strlen(recurse)-1]!=':')&&(recurse[strlen(recurse)-1]!='\\'))
   strcat(recurse, "\\");
  strcat(recurse, "*.*");
  t=dos_findfirst(recurse,ATTR_DIRECTORY,ffblk);
  while (t)
  {
   if (*(ffblk->fnam)!='.')
   {
    if (ffblk->attr&ATTR_DIRECTORY)
    {
     int e;
     
     strcpy(recurse, target);
     if ((recurse[strlen(recurse)-1]!=':')&&(recurse[strlen(recurse)-1]!='\\'))
      strcat(recurse, "\\");
     strcat(recurse, ffblk->fnam);
     replace(source, recurse);
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

/*
 * We know we have a switch; record it, if valid.
 * If not, die screaming.
 */
int getswitch (char x)
{
 switch (x)
 {
#ifdef HELP
  case '?':
   dos_puts(m_help);
   return -1;
#endif
  case 'A':
   mode|=MODE_A;
   return 0;
  case 'P':
   mode|=MODE_P;
   return 0;
  case 'R':
   mode|=MODE_R;
   return 0;
  case 'S':
   mode|=MODE_S;
   return 0;
  case 'W':
   mode|=MODE_W;
   return 0;
  case 'U':
   mode|=MODE_U;
   return 0;
  case 'D':
   mode|=MODE_D;
   return 0;
  default:
   dos_puts (m_switch);
   dos_putc (x);
   dos_puts (crlf);
   return -1;
 }
}

int main (int argc, char **argv)
{
 char *targ;
 char *ptr;
 int t;
 unsigned short dosver;
 
 /* Require MS-DOS 3 because of how we do our sanity check */
 _asm mov ah, 0x30;
 _asm int 0x21;
 _asm xchg ah, al;
 _asm mov dosver, ax;
 if (dosver<0x0300)
 {
  dos_puts(m_olddos);
  return EXIT_OLDDOS;
 }

 /* Set defaults. */
 targ=srcmask;
 mode=retval=tally=0;
 
 /* Scream if no argument specified. */
 if (argc==1)
 {
  dos_puts(m_noargs);
  return EXIT_SYNERR;
 }
 
 /*
  * Traipse through the argument list somewhat more DOS-style than Unix-style.
  * Note that this diddles argv, usually a Very Bad Thing(R)(TM)(C), but it
  * makes our parsing easier.
  */
 for (t=1; t<argc; t++)
 {
  /* If we have switches, split the argument, and parse everything after. */
  ptr=strchr(argv[t],'/');
  if (ptr)
  {
   *ptr=0;
   
   /* If a switch caused an error, then die (we already screamed). */
   if (getswitch(smash(ptr[1]))) return EXIT_SYNERR;
   
   /* Skip over the switch. */
   ptr+=2;
   
   /*
    * Keep checking for switches. Anything else in this part of the argument
    * is garbage and will be treated as such.
    */
   while (*ptr)
   {
    if (*ptr=='/')
    {
     if (!ptr[1])
     {
      dos_puts(m_synerr);
      return EXIT_SYNERR;
     }
     if (getswitch(smash(ptr[1])))
      return EXIT_SYNERR;
     else
      ptr+=2;
    }
    else
    {
     dos_puts(m_synerr);
     return EXIT_SYNERR;
    }
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
 
 /* The /S, /U and /D switches are incompatible with /A. */
 if (mode&MODE_A)
 {
  if (mode&(MODE_S|MODE_U|MODE_D))
  {
   dos_puts(m_clash);
   return EXIT_SYNERR;
  }
 }
 
 /* If we did not get a source argument, then die screaming. */
 if (targ==srcmask)
 {
  dos_puts(m_noargs);
  return EXIT_SYNERR;
 }

 /* 
  * If we did not get a target argument, just assume the current directory.
  * If we ARE using the current directory, fill it in.
  * (This is necessary because sometimes using "." will fail, e.g., if the
  *  working directory is "\".)
  */
 if (targ)
 {
  targ=0;
  strcpy(tgtmask, ".");
 }

 if (!strcmp(tgtmask, "."))
 {
  char *xt;
  
  xt=tgtmask+1;
  
  _asm mov ah, 0x47;
  _asm mov dl, 0x00; /* current drive */
  _asm mov si, xt;
  _asm int 0x21;
  *tgtmask='\\';
 }
 
 /* Make sure that the target is a valid folder. */
 strcpy(tgtgen, tgtmask);
 if (tgtgen[strlen(tgtgen)-1]=='\\') tgtgen[strlen(tgtgen)-1]=0;
 strcat(tgtgen, "\\NUL");
 t=dos_open(tgtgen, 0);
 if (t)
  dos_close(t);
 else
 {
  wrerr();
  return retval;
 }
 
 /* Smash the case of our arguments for display. */
 for (t=0; srcmask[t]; t++) srcmask[t]=smash(srcmask[t]);
 for (t=0; tgtmask[t]; t++) tgtmask[t]=smash(tgtmask[t]);
 
 /*
  * If the user wants us to wait (for a disk swap?) before actually carrying
  * out the operation, then oblige.
  */
 if (mode&MODE_W) pause();
 
 replace(srcmask, tgtmask);

 summarize(); 
 return (tally?retval:EXIT_NOFILE);
}
