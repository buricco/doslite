/*
 * Copyright 2022 S. V. Nickolas.
 * CRC32 table and algorithm copyright 1986 Gary S. Brown.
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

static unsigned long crc32_tab[] = {
 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

unsigned long crc32(unsigned long crc, const void *buf, unsigned size)
{
 const unsigned char *p;

 p = buf;
 crc ^= 0xFFFFFFFF;

 while (size--)
  crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

  return crc ^ 0xFFFFFFFF;
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
