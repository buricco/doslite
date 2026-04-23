/*
 * Copyright 2022, 2026 S. V. Nickolas.
 * Checksum routine written by Jeffrey H. Johnson;
 *   supplied to S. V. Nickolas as public domain April 22, 2026.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the Software), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * If you set the stack size too small, the program will crash with a stack
 * overflow, but if you set it too large, it will crash with out of memory.
 * 
 * Current advice: use wcl -k12288 (12K stack).
 */

/*
 * Important note:
 *   Although the syntax and most of the superficiality of DOSLITE's CRC is
 *   the same as IBM's, it is a different algorithm (and also is not the SFV
 *   CRC32 algorithm, if output is anything to go by).  Do not use the CRCs
 *   output by one implementation of this tool against those output by a
 *   different implementation unless you know they use the same algorithm!
 * 
 * (This may also be a bug or a 16/32 issue.)
 */

#include <dos.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUFSIZ 8192

/*
 * It is clearer to use a macro every time we return this code, than to write
 * down a magic number for the same purpose.
 */
#define EXIT_SYNERR 1

const char *m_noargs  = "Required parameter missing\r\n",
           *m_args    = "Too many parameters\r\n",
           *m_synerr  = "Syntax error\r\n",
           *m_ram     = "Insufficient memory\r\n",
           *m_nofiles = "No files found\r\n",
           *m_switch  = "Invalid switch - /",
           *m_crc     = "CRC=",
           *m_more    = "-- More --",
           *m_nomore  = "\r          \r",
#ifdef HELP
           *m_help    =
"Displays the Cyclic Redundancy Check (CRC) of a file.\r\n\r\n"
"CRC [drive:][path][filespec] [/S] [/P]\r\n\r\n"
"  [drive:][path][filespec] Specify the directory and file.\r\n"
"  /S      Processes files in all directories in the specified path.\r\n"
"  /P      Pauses after each screen of information.\r\n",
#endif
           *m_crlf="\r\n";

/* Error code returned from MS-DOS. */
int dos_errno;

unsigned char flags;

int maxrow;
int currow;

#define MODE_S 0x01
#define MODE_P 0x02
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

int dos_write (int handle, const void *buf, unsigned len)
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
 unsigned short a, b;
 unsigned long c;
 a=(pos>>16);
 b=(pos&0xFFFF);

 flags=0;
 _asm
 {
  mov ah, 0x42;
  mov al, mode;
  mov bx, handle;
  mov cx, a;
  mov dx, b;
  int 0x21;
  jnc lseekok;
  mov flags, 0xFF;
  mov dos_errno, ax;
  jmp short lseekend;
lseekok:
  mov a, dx;
  mov b, ax;
lseekend:
 };
 if (flags) return -1;

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

void wrhex4 (unsigned long x)
{
 wrhex1(x>>24);
 wrhex1((x>>16)&0xFF);
 wrhex1((x>>8)&0xFF);
 wrhex1(x&0xFF);
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

#define POLYNOMIAL 0x51F9D3DEUL

unsigned long crc32 (unsigned long crc, const unsigned char *buf, size_t size)
{
 size_t t;
 
 for (t=0; t<size; t++)
 {
  unsigned long c;
  int j;
  
  c=buf[t];
  
  crc ^= c<<24;
  
  for (j=0; j<8; j++)
  {
   if (crc & 0x80000000)
    crc=((crc<<1)&0xFFFFFFFF)^POLYNOMIAL;
   else
    crc=((crc<<1)&0xFFFFFFFF);
  }
 }
 
 return crc;
}

unsigned long crcfile (char *filename)
{
 long l;
 unsigned long roll;
 int file;
 
 unsigned char buf[MAXBUFSIZ];
 
 file=dos_open(filename,0);
 if (!file)
 {
  wrerr();
  exit(1);
 }
 roll=0;
 l=dos_lseek(file,0,2);
 dos_lseek(file,0,0);
 while (l>MAXBUFSIZ)
 {
  dos_read(file,buf,MAXBUFSIZ);
  roll=crc32(roll,buf,MAXBUFSIZ);
  l-=MAXBUFSIZ;
 }
 if (l)
 {
  dos_read(file,buf,l);
  roll=crc32(roll,buf,l);
 }
 dos_close(file);
 return roll;
}

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
}

void process (char *filename)
{
 char *p;
 int s;
 
 p=strrchr(filename, '\\');
 if (!p)
 {
  p=filename;
  if (p[1]==':') p+=2;
 }
 s=17-strlen(p);
 dos_puts(filename);
 while (s--) dos_putc(' ');
 dos_puts(m_crc);
 wrhex4(crcfile(filename));
 dos_puts(m_crlf);
}

/*
 * The gories.
 * 
 * Since we recurse, we need to be able to pass a filename, but the other
 * arguments are set at parse time and will not change during execution.
 */
int traipse (char *path)
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
 p=strrchr(path, '\\');
 if (p)
 {
  strcpy(dirname, path);
  (strrchr(dirname, '\\'))[1]=0;
  p++;
 }
 else
 {
  if (path[1]==':')
  {
   dirname[0]=path[0];
   dirname[1]=':';
   dirname[2]=0;
   p=path+2;
  }
  else
  {
   dirname[0]='.';
   dirname[1]='\\';
   dirname[2]=0;
   p=path;
  }
 }

 /*
  * Handle the easier case first.
  * (No wildcards)
  */
 if (!(strchr(path, '?')||strchr(path, '*')))
 {
  process(path);
 }
 else
 {
  /*
   * Find all files on which we might want to operate.
   * (See above for how we figure this out.)
   */
  t=dos_findfirst(path,0,ffblk);
  if (!t)
  {
   e=dos_errno;
   dos_puts(path);
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

   process(filename);
   
   currow++;
   if (mode&MODE_P)
   {
    if (currow==(maxrow-1))
    {
     dos_puts(m_more);
     _asm mov ah, 0x08;       /* GETCH */
     _asm int 0x21;
     dos_puts(m_nomore);
     currow=0;
    }
   }
  
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
     traipse(recurse);
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

/* Actually returns max row minus 1 */
unsigned getmaxrow (void)
{
 unsigned t;
 
 _asm mov ah, 0x12;           /* do we have an EGA? */
 _asm mov bx, 0xFF10;
 _asm mov cx, 0xFFFF;
 _asm int 0x10;
 _asm mov t, cx;
 if (t==0xFFFF) return 24;
 return (*(char far*)MK_FP(0x0040, 0x0084));
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
  case 'P':
   mode|=MODE_P;
   return 0;
  case 'S':
   mode|=MODE_S;
   return 0;
  default:
   dos_puts (m_switch);
   dos_putc (x);
   dos_puts (m_crlf);
   return -1;
 }
}

int main (int argc, char **argv)
{ 
 char *myarg;
 char *ptr;
 int t;

 maxrow=getmaxrow();
 currow=0;
 myarg=0;

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
  if (myarg)
  {
   dos_puts(m_args);
   return EXIT_SYNERR;
  }
  myarg=argv[t];
 }
 
 return traipse(myarg);
}
