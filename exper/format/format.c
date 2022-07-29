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
 * 1. Is the drive removable?
 * 2. Can we configure the drive for the settings we want?
 *    (Removable drives only.)
 * 3. Allocate space for the FAT in progress.
 * 4. Prompt user for final confirmation.
 * 5. Zot the disk.
 * 6. Write the boot sector, FAT and root directory.
 * 7. Prompt, if appropriate, for a volume label.
 * 8. Display final statistics.
 * 
 * Note that unlike most DOSLITE commands this program will be missing a bit
 * of the functionality of the DOS 5+ version:
 *   * /U will be supported but ignored.
 *   * /S will not be supported; this may change once a kernel is available.
 *   * /C (Win9x and PC DOS 7) will not be supported.  It reverted a speed
 *        hack and restored original behavior which we are already defaulting
 *        to, so it is unneeded.
 *   * Undocumented switches will not be supported.
 *     However, MS-DOS 3.3's untitled /H switch (which became /BACKUP in 4.0)
 *     is useful enough that I do intend to include it.
 */

#include <stdlib.h>
#include <string.h>

typedef  unsigned char   byte;
typedef  unsigned short  word;
typedef  unsigned long   dword;

const char *e_noargs   = "Required parameter missing\r\n",
           *e_args     = "Too many parameters\r\n",
           *e_switch   = "Invalid switch - /",
           *e_synerr   = "Syntax error\r\n",
           *e_parm     = "Invalid parameter\r\n",
           *e_tn       = "Must enter both /T and /N parameters\r\n",
           *e_voldos1  = "DOS 1.x format does not support volume labels\r\n",
           *e_fixed    = "Parameter(s) not compatible with fixed disk\r\n",
           *e_notme    = "Parameter(s) not compatible with drive\r\n",
           *e_ioctl    = "Error in IOCTL call\r\n",
           *e_subst    = "Cannot FORMAT an ASSIGNed or SUBSTed drive\r\n",
           *e_remote   = "Cannot FORMAT a network drive\r\n",
           *e_drive    = "Invalid drive specification\r\n",
           *e_ready    = "Drive not ready\r\n",
           *e_wrprot   = "Disk write protected\r\n",
           *e_trk0     = "Invalid media or track 0 bad - disk unusable\r\n",
           *e_confirm1 = "Insert new diskette for drive ",
           *e_confirm2 = ":\r\nand press <Enter> when ready. ",
           *e_confirm3 = "WARNING: All data on non-removable drive ",
           *e_confirm4 = ": will be lost!\r\n"
                         "Proceed with format (Y/N)? ",
           *e_invlabel = "Invalid characters in volume label\r\n",
           *e_long     = "Too many characters in volume label\r\n",
           *e_percent  = " percent of format complete\r", /* no newline */
           *e_complete = "Format complete               \r\n",
           *e_failure  = "Format aborted\r\n",
#ifdef HELP
           *e_help     =
"Formats a disk for use with DOS.\r\n\r\n"
"FORMAT drive: [/V[:label]] [/Q] [/F:size]\r\n"
"FORMAT drive: [/V[:label]] [/Q] [/T:tracks /N:sectors]\r\n"
"FORMAT drive: [/V[:label]] [/Q] [/1] [/4]\r\n"
"FORMAT drive: [/Q] [/1] [/4] [/8]\r\n\r\n"
"  /V[:label]  Specifies the volume label.\r\n"
"  /Q          Performs a quick format.\r\n"
"  /F:size     Specifies the size of the floppy disk to format (such \r\n"
"              as 160, 180, 320, 360, 720, 1.2, 1.44, 2.88).\r\n"
"  /T:tracks   Specifies the number of tracks per disk side.\r\n"
"  /N:sectors  Specifies the number of sectors per track.\r\n"
"  /1          Formats a single side of a floppy disk.\r\n"
"  /4          Formats a 5.25-inch 360K floppy disk in a high-density drive.\r\n"
"  /8          Formats eight sectors per track.\r\n",
#endif
           *e_crlf     = "\r\n";

/*
 * Generic dummy bootsector. (A fix will be needed when the kernel is ready!)
 * 
 * Contains only the JMP, OEMID, and a fake IPL which merely displays the 
 * famous error message.  Leave OEMID as "DOSLT5.0" - changing the 5 may break
 * things in obscure programs.
 */
const char bootsec[512]={
 0xEB, 0x3C, 0x90, 0x44, 0x4F, 0x53, 0x4C, 0x54, 
 0x35, 0x2E, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0xC0,
 0x8E, 0xD0, 0x8E, 0xD8, 0x8E, 0xC0, 0xBC, 0x00,
 0x7C, 0xBE, 0x61, 0x7C, 0xB4, 0x0E, 0x8A, 0x04,
 0x08, 0xC0, 0x74, 0x07, 0x30, 0xFF, 0xCD, 0x10,
 0x46, 0xEB, 0xF1, 0x30, 0xE4, 0xCD, 0x16, 0xCD,
 0x19, 0x0D, 0x0A, 0x4E, 0x6F, 0x6E, 0x2D, 0x73,
 0x79, 0x73, 0x74, 0x65, 0x6D, 0x20, 0x64, 0x69,
 0x73, 0x6B, 0x20, 0x6F, 0x72, 0x20, 0x64, 0x69,
 0x73, 0x6B, 0x20, 0x65, 0x72, 0x72, 0x6F, 0x72,
 0x0D, 0x0A, 0x52, 0x65, 0x70, 0x6C, 0x61, 0x63,
 0x65, 0x20, 0x61, 0x6E, 0x64, 0x20, 0x70, 0x72,
 0x65, 0x73, 0x73, 0x20, 0x61, 0x20, 0x6B, 0x65,
 0x79, 0x20, 0x77, 0x68, 0x65, 0x6E, 0x20, 0x72,
 0x65, 0x61, 0x64, 0x79, 0x0D, 0x0A, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
};

char linbuf[131];

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

/*
 * /F will be translated into /T+/N, with /1 being set for 160 and 180,
 * /8 set for 160 and 320. (Different parameters are needed for 160 and 320.)
 */
#define MODE_V 0x01
#define MODE_U 0x02
#define MODE_Q 0x04
#define MODE_H 0x08
#define MODE_1 0x10
#define MODE_8 0x80
byte mode;
word slash_t, slash_n;
byte vollab[12];
byte *invchars="\"&()*+,./:;<=>?[\]^|";
byte construct[10];

byte isrem;

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

struct bpb
{
 unsigned short bpsec;
 unsigned char spclus;
 unsigned short ressec;
 unsigned char numfats;
 unsigned short dirents;
 unsigned short sectors;
 unsigned char mediatype;
 unsigned short spfat;
 unsigned short sptrk;
 unsigned short heads;
 unsigned long hidden;
 unsigned long lsectors;
};

struct drivparm
{
 byte specfunc;               /* 0x02 always on, 0x01 to replace BPB */
 byte devtype;                /* 0=5.25DD, 1=5.25HD, 2=3.5DD, 7=3.5HD, 9=3.5ED
                                 and others are special formats */
 word devattr;                /* 0x01 fixed disk, 0x02 detect open door */
 word cyls;
 byte golow;                  /* 0x01 /4, otherwise 0x00 */
 struct bpb bpb;
};

struct fmtblock
{
 byte multrak;
 word head;
 word cyl;
 word notrk;
};

#define NUM_BPB 8
struct bpb fdbpb[NUM_BPB]=
{
 {512, 1, 1, 2,  64,  320, 0xFE, 1,  8, 1, 0, 0}, /* 160 KB */
 {512, 1, 1, 2,  64,  360, 0xFC, 2,  9, 1, 0, 0}, /* 180 KB */
 {512, 2, 1, 2, 112,  640, 0xFF, 1,  8, 2, 0, 0}, /* 320 KB */
 {512, 2, 1, 2, 112,  720, 0xFD, 2,  9, 2, 0, 0}, /* 360 KB */
 {512, 2, 1, 2, 112, 1440, 0xF9, 3,  9, 2, 0, 0}, /* 720 KB */
 {512, 1, 1, 2, 224, 2400, 0xF9, 7, 15, 2, 0, 0}, /* 1.2 MB */
 {512, 1, 1, 2, 224, 2880, 0xF0, 9, 18, 2, 0, 0}, /* 1.44 MB */
 {512, 2, 1, 2, 240, 5760, 0xF0, 9, 36, 2, 0, 0}, /* 2.88 MB */
};

struct drivparm drivparm;

byte huge *fat;

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

/* Case smash input. */
char smash (char x)
{
 if ((x<'a')||(x>'z')) return x;
 return x&0x5F;
}

int sanitycheck (byte drive)
{
 word attrword;
 
 _asm mov ax, 0x4409
 _asm mov bl, drive;
 _asm inc bl;
 _asm int 0x21;
 _asm mov dx, attrword;
 _asm lahf;
 _asm mov flags, ah;
 
 if (flags&1)
 {
  _asm mov attrword, ax;
  
  if (attrword==0x000F)
   dos_puts(e_drive);
  else if (attrword==0x0015)
   dos_puts(e_ready);
  else
   dos_puts(e_ioctl);
  return 0;
 }
 
 if (attrword&0x8000)
 {
  dos_puts(e_subst);
  return 0;
 }
 
 if (attrword&0x1000)
 {
  dos_puts(e_remote);
  return 0;
 }
 
 return 1;
}

int getparms (byte drive)
{
 void *drivparm_ptr=&drivparm;
 
 _asm mov ax, 0x440D;
 _asm mov bl, drive;
 _asm inc bl;
 _asm mov cx, 0x0860;
 _asm mov dx, drivparm_ptr;
 _asm int 0x21;
 
 _asm lahf;
 _asm mov flags, ah;
 
 if (flags&1)
 {
  dos_puts(e_ioctl);
  return 0;
 }
 
 isrem=drivparm.devattr&0x01;
 
 return 1;
}

/*
 * Skip this process if /H was specified.
 */
int confirm (byte drive)
{
 char *linbuf_ptr=linbuf;

 dos_puts (isrem?e_confirm1:e_confirm3);
 dos_putc (drive+'A');
 dos_puts (isrem?e_confirm2:e_confirm4);

 *linbuf=128;                /* Length of line buffer */
 linbuf[1]=0;                /* Disable F3 */
 _asm mov ah, 0x0A;
 _asm mov dx, linbuf_ptr;
 _asm int 0x21;
 
 if (!isrem)
  if (smash(linbuf[2])!='Y') return 0;
 
 return 1;
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

int zot (byte drive, word heads, word cylinders)
{
 int e;
 struct fmtblock fmtblock;
 word head, cylinder;
 byte percent;

 void *fmtblock_ptr=&fmtblock;
 
 fmtblock.multrak=fmtblock.notrk=0;
 
 for (cylinder=0; cylinder<cylinders; cylinder++)
 {
  for (head=0; head<heads; head++)
  {
   percent = (100*((cylinder*heads)+heads))/(cylinders*heads);
   dos_putc('\r');
   if (percent<10) dos_putc(' ');
   if (percent<100) dos_putc(' ');
   wrnum(percent);
   dos_puts(e_percent);
   
   fmtblock.head=head;
   fmtblock.cyl=cylinder;
   
   _asm mov ax, 0x440D;
   _asm mov cx, 0x0842;
   _asm mov dx, fmtblock_ptr;
   _asm int 0x21;
   _asm lahf;
   _asm mov flags, ah;
   if (flags&1)
   {
    _asm mov ah, 0x59;
    _asm xor bx, bx;
    _asm int 0x21;
    _asm mov e, ax;
    
    switch (e)
    {
     case 0x13:
      dos_puts(e_wrprot);
      return -1;
     case 0x15:
      dos_puts(e_ready);
      return -1;
     case 0x17:
     case 0x1B:
     case 0x1D:
     case 0x1E:
     case 0x1F:
      if (!cylinder)
      {
       dos_puts(e_trk0);
       return 1;
      }
      /* All: Bad sector */
      break;
     default:
      dos_puts(e_ioctl);
      return 1;
    }
   }
  }
 }

 dos_puts(e_complete);
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
  case 'Q':
   mode |= MODE_Q;
   break;
  case 'U':
   mode |= MODE_U;
   break;
  case 'H':
   mode |= MODE_H;
   break;
  case '1':
   mode |= MODE_1;
   slash_t = 40;
   break;
  case '4':
   slash_t = 40;
   slash_n = 9;
   break;
  case '8':
   mode |= MODE_8;
   slash_t = 40;
   slash_n = 8;
   break;
  default:
   dos_puts(e_switch);
   dos_putc(a);
   dos_puts(e_crlf);
   return -1;
 }

 return 0;
}

word getnum (char *x)
{
 word y;
 
 y=0;
 
 if ((*x<'0')||(*x>'9'))
 {
  dos_puts(e_parm);
  exit (1);
 }
 
 while ((*x>='0')&&(*x<='9'))
 {
  if (y>6552)
  {
   dos_puts(e_parm);
   exit (1);
  }
  
  y*=10;
  
  y+=((*x)&0x0F);
  x++;
 }
 
 return y;
}

int main (int argc, char **argv)
{
 byte drive;
 char *ptr;
 int t;

 /* Set defaults. */
 drive=255;
 mode=0;
 *vollab=0;

 /* Scream if no argument specified. */
 if (argc==1)
 {
  dos_puts(e_noargs);
  return 1;
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

   if (smash(*ptr)=='F')
   {
    word s;
    byte cc;
    
    s=0;
    cc='K';
    ptr++;
    if (*ptr==':') ptr++;
    if (!strncmp(ptr, "1.2", 3))
    {
     s=1200;
     cc='M';
     ptr+=3;
    }
    else if (!strncmp(ptr, "1.44", 4))
    {
     s=1440;
     cc='M';
     ptr+=4;
    }
    else if (!strncmp(ptr, "2.88", 4))
    {
     s=2880;
     cc='M';
     ptr+=4;
    }
    else
    {
     s=getnum(ptr);
     while ((*ptr>='0')&&(*ptr<='9')) ptr++;
    }
    if (smash(*ptr)==cc)
    {
     ptr++;
     if (smash(*ptr)=='B') ptr++;
    }
    
    switch (s)
    {
     case 160:
      mode |= (MODE_1 | MODE_8);
      slash_t=40; slash_n=8;
      break;
     case 180:
      mode |= MODE_1;
      slash_t=40; slash_n=9;
      break;
     case 320:
      mode |= MODE_8;
      slash_t=40; slash_n=8;
      break;
     case 360:
      slash_t=40; slash_n=9;
      break;
     case 720:
      slash_t=80; slash_n=9;
      break;
     case 1200:
      slash_t=80; slash_n=15;
      break;
     case 1440:
      slash_t=80; slash_n=18;
      break;
     case 2880:
      slash_t=80; slash_n=36;
      break;
     default:
      dos_puts(e_parm);
      return 1;
    }
   }
   if (smash(*ptr)=='V')
   {
    mode |= MODE_V;
    ptr++;
    
    if (!((*ptr=='/')||(*ptr==' ')||(!*ptr)))
    {
     int l, i;
     
     memset(vollab,0,12);
     
     while ((*ptr!='/')&&(*ptr!=' ')&&(*ptr))
     {
      if (l>11)
      {
       dos_puts(e_long);
       return 1;
      }
      for (i=0; invchars[i]; i++)
       if (*ptr==invchars[i])
       {
        dos_puts(e_invlabel);
        return 1;
       }
      vollab[l++]=smash(*ptr++);
     }
    }
   }
   if (smash(*ptr)=='N')
   {
    if (*(++ptr)==':') ptr++;
    
    slash_n=getnum(ptr);
    while ((*ptr>='0')&&(*ptr<='9')) ptr++;
   }
   if (smash(*ptr)=='T')
   {
    if (*(++ptr)==':') ptr++;
    
    slash_t=getnum(ptr);
    while ((*ptr>='0')&&(*ptr<='9')) ptr++;
   }
   else if (getswitch(smash(*ptr)))
    return 1;
   ptr++;
   if (!*ptr) continue;
   while (*ptr)
   {
    if (*ptr!='/')
    {
     dos_puts(e_synerr);
     return 1;
    }
    ptr++;
    if (getswitch(smash(*ptr))) return 1;
    ptr++;
   }
  }

  /* If the beginning of the argument was /, it is now a NUL terminator. */
  if (!*argv[t]) continue;
  
  if (drive!=255)
  {
   dos_puts(e_args);
   return 1;
  }
  
  if (!strcmp(&(argv[t][1]), ":"))
   drive=smash(*argv[t]-'A');
  else
  {
   dos_puts(e_parm);
   return 1;
  }
 }
 
 if ((slash_t||slash_n)&&(!(slash_t&&slash_n)))
 {
  dos_puts(e_tn);
  return 1;
 }
 
 if ((mode&(MODE_V|MODE_8))==(MODE_V|MODE_8))
 {
  dos_puts(e_voldos1);
  return 1;
 }

 getparms(drive);
 if ((!(isrem))&&((mode&(MODE_1|MODE_8))||(slash_n)))
 {
  dos_puts(e_fixed);
  return 1;
 }
 
 if ((!slash_t)||((slash_t==drivparm.cyls)&&(slash_n==drivparm.bpb.sptrk)))
 {
  /* No reconfiguration needed */
 }
 else
 {
  /*
   * MODE_1 is only valid on drive types 0 and 1.
   * 
   * 0 = accepts T=40, N=8 or 9.
   * 1 = accepts T=40, N=8 or 9; T=80, N=15
   * 2 = accepts T=80, N=9 (only)
   * 7 = accepts T=80, N=9 or 18.
   * 9 = accepts T=80, N=9, 18 or 36.
   */
  if (slash_t)
  {
   byte bad;
  
   bad=0;
   if ((drivparm.devtype>1)&&(mode&MODE_1)) bad=1;
   switch (drivparm.devtype)
   {
    case 1:
     if ((slash_t==80)&&(slash_n==15)) break;      /* FALL THROUGH */
    case 0:
     if (slash_t!=40) bad=1;
     if ((slash_n!=8)&&(slash_n!=9)) bad=1;
     break;
    case 9:
     if ((slash_t==80)&&(slash_n==36)) break;      /* FALL THROUGH */
    case 7:
     if ((slash_t==80)&&(slash_n==18)) break;      /* FALL THROUGH */
    case 2:
     if ((slash_t==80)&&(slash_n==9)) break;
     bad=1;
     break;
   }
  
   if (bad)
   {
    dos_puts(e_notme);
    return 1;
   }
  }
 }
 
 if (slash_n)
 {
  byte bad;
  
  bad=1;
  for (t=0; t<NUM_BPB; t++)
  {
   if ((slash_n==fdbpb[t].sptrk)&&(fdbpb[t].heads=(mode&MODE_1)?1:2))
   {
    bad=0;
    memcpy(&(drivparm.bpb), &(fdbpb[t]), sizeof(struct bpb));
    drivparm.cyls=slash_t;
   }
   
   if (bad)
   {
    dos_puts(e_notme);
    return 1;
   }
  }
 }
 
 return 0;
}
