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
 * No switches are officially documented, but the /MBR and /STATUS switches
 * are well-known, so these should be supported.
 * 
 * Unlike MS-DOS and PC DOS, we use DOS APIs as much as possible for console
 * I/O, which necessitates some changes to the look and feel, but regardless,
 * the program should still be similar enough to those that muscle memory can
 * take over.
 */

#include <dos.h>
#include <string.h>

#define FDISK_VER   "0.00"
#define FDISK_CPR   "Copyright 2022 S. V. Nickolas"

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

const char *e_nohds      = "No fixed disks present\r\n",
           *e_args       = "Too many parameters\r\n",
           *e_mustreboot = "\r\n\System must now reboot.\r\n"
                           "Please ensure a bootable disk is present.\r\n"
                           "Press any key to continue... ",
           *e_banner     = "DOSLITE Fixed Disk Setup Program v" FDISK_VER
                           "\r\nBuild  " __DATE__ " " __TIME__ "\r\n"
                           FDISK_CPR "\r\n\r\n",
           *e_current    = "Current fixed disk drive: ",
           *e_main0      = "FDISK Options\r\n\r\n",
           *e_main1      = "1. Create DOS partition or logical DOS drive\r\n"
                           "2. Set active partition\r\n"
                           "3. Delete DOS partition or logical DOS drive\r\n"
                           "4. Display partition information\r\n",
           *e_main2      = "5. Change current fixed disk drive\r\n",
           *e_main3      = "0. Exit to DOS\r\n\r\n",
           *e_choose     = "   Enter selection: ",
           *e_noboot     = "WARNING: There is no active partition on the first"
                           " hard disk.  You will not be\r\n         able to"
                           " boot from hard disk until a partition is made"
                           " active.\r\n\r\n",
#ifdef HELP
           *e_help       = "Configures a fixed disk to work with DOS.\r\n"
                           "\r\n"
                           "FDISK\r\n",
#endif
           *e_crlf       = "\r\n";

struct pte 
{
 byte active;
 byte start_chs[3];
 byte os;
 byte end_chs[3];
 dword start_lba;
 dword len_lba;
};

struct mbr 
{
 byte bootloader [0x01BE];
 struct pte pte[4];
 word signature;
};

/*
 * Since $00 is actually valid, we have to consider the *second* $00 to be the
 * end-of-list mark, and the string associated with it the default to report
 * if we find something we don't recognize.
 */
struct ptype
{
 byte type;
 byte string[8];
} ptype[]={
 0x00,"Pri DOS",
 0x01,"Pri DOS",
 0x02," Xenix ",
 0x03," Xenix ",
 0x04,"Pri DOS",
 0x05,"Ext DOS",
 0x06,"Pri DOS",
 0xFF," Table ",
 0xDB," PC/IX ",
 0x07," HPFS  ",
 0x64,"Novell ",
 0x75,"CP/M-86",
 0x00,"Non-DOS",
};

byte nohd, nohds;
byte hdtab[128];
byte linbuf[131];
byte buf[128];
byte must_reboot;

struct mbr currentmbr;

/* Error code returned from MS-DOS. */
int dos_errno;

byte flags;

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
 * Slightly tweaked and rebuilt version of the standard MBR bootloader.
 */
byte mbrboot[]={
 0xFA, 0x31, 0xC0, 0x8E, 0xD0, 0xBC, 0x00, 0x7C, 
 0x89, 0xE6, 0x50, 0x07, 0x50, 0x1F, 0xFB, 0xFC,
 0xBF, 0x00, 0x06, 0xB9, 0x00, 0x01, 0xF3, 0xA5, 
 0xEA, 0x1D, 0x06, 0x00, 0x00, 0xBE, 0xBE, 0x07,
 0xB3, 0x04, 0x80, 0x3C, 0x80, 0x74, 0x0E, 0x80, 
 0x3C, 0x00, 0x75, 0x1C, 0x83, 0xC6, 0x10, 0xFE,
 0xCB, 0x75, 0xEF, 0xCD, 0x18, 0x8B, 0x14, 0x8B, 
 0x4C, 0x02, 0x89, 0xF5, 0x83, 0xC6, 0x10, 0xFE,
 0xCB, 0x74, 0x1A, 0x80, 0x3C, 0x00, 0x74, 0xF4, 
 0xBE, 0x8B, 0x06, 0xAC, 0x08, 0xC0, 0x74, 0x0B,
 0x56, 0xBB, 0x07, 0x00, 0xB4, 0x0E, 0xCD, 0x10, 
 0x5E, 0xEB, 0xF0, 0xEB, 0xFE, 0xBF, 0x05, 0x00,
 0xBB, 0x00, 0x7C, 0xB8, 0x01, 0x02, 0x57, 0xCD,
 0x13, 0x5F, 0x73, 0x0C, 0x31, 0xC0, 0xCD, 0x13,
 0x4F, 0x75, 0xED, 0xBE, 0xA3, 0x06, 0xEB, 0xD3,
 0xBE, 0xC2, 0x06, 0xBF, 0xFE, 0x7D, 0x81, 0x3D,
 0x55, 0xAA, 0x75, 0xC7, 0x89, 0xEE, 0xEA, 0x00,
 0x7C, 0x00, 0x00, 0x49, 0x6E, 0x76, 0x61, 0x6C,
 0x69, 0x64, 0x20, 0x70, 0x61, 0x72, 0x74, 0x69,
 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x74, 0x61, 0x62,
 0x6C, 0x65, 0x00, 0x45, 0x72, 0x72, 0x6F, 0x72,
 0x20, 0x6C, 0x6F, 0x61, 0x64, 0x69, 0x6E, 0x67,
 0x20, 0x6F, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69,
 0x6E, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65,
 0x6D, 0x00, 0x4D, 0x69, 0x73, 0x73, 0x69, 0x6E,
 0x67, 0x20, 0x6F, 0x70, 0x65, 0x72, 0x61, 0x74,
 0x69, 0x6E, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74,
 0x65, 0x6D, 0x00
};

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

void wrnum (unsigned long x)
{
 char n[11];
 char *p;
 
 if (!x)
 {
  dos_putc('0');
  return;
 }
 
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

int getmbr (void)
{
 byte e;
 byte rd;
 void *mbrptr;
 
 rd=hdtab[nohd];
 mbrptr=&currentmbr;
 
 /* Reset disk */
 _asm xor ah, ah;
 _asm mov dl, rd;
 _asm int 0x13;
 _asm lahf;
 _asm mov e, ah;
 if (e&1) return 0;
 
 _asm mov ax, 0x0201;         /* read 1 sector */
 _asm mov cx, 0x0001;         /* cyl 0, sec 1 */
 _asm xor dh, dh;             /* head 0 */
 _asm mov dl, rd;
 _asm mov bx, mbrptr;
 _asm int 0x13;
 _asm lahf;
 _asm mov e, ah;
 return !(e&1);
}

void showparts (void)
{
 
}

void mainmenu (void)
{
 byte c;
 
 while (1)
 {
  if (nohds!=1)
  {
   dos_puts(e_current);
   wrnum(nohd+1);
   dos_puts(e_crlf);
   dos_puts(e_crlf);
  }
  dos_puts(e_main0);
  dos_puts(e_main1);
  if (nohds!=1) dos_puts(e_main2);
  dos_puts(e_main3);
  c=255;
  
  if (!nohd)
  {
   byte xyz;
   
   xyz=((currentmbr.pte[0].active&0x80)||
        (currentmbr.pte[1].active&0x80)||
        (currentmbr.pte[2].active&0x80)||
        (currentmbr.pte[3].active&0x80));
   
   if (!xyz) dos_puts(e_noboot);
  }
  
  do
  {
   dos_puts(e_choose);
   dos_getlin(buf, 128);
   if (*buf=='0') return;
   if ((*buf>'0')&&(*buf<'5')) c=*buf-'0';
   if ((*buf=='5')&&(nohds!='1')) c=5;
  } while (c==255);
  dos_puts(e_crlf);
  
  switch (c)
  {
   case 1:
    break;
   case 2:
    break;
   case 3:
    break;
   case 4:
    break;
   case 5:
    break;
  }
 }
}

void reboot (void)
{
 dos_puts(e_mustreboot);
 dos_getche();
 _asm db 0xEA;                /* JMP FAR */
 _asm dd 0xFFFF0000;          /* 8086 start address */
}

/*
 * Code for handling the undocumented /MBR switch (MS-DOS/PC DOS 5.0+).
 * 
 * If the MBR signature is present, this is relatively nondestructive; it will
 * not step on the partition table.  Otherwise, it will zero the partition
 * table before copying in the boot code.
 */
void zot_mbr (void)
{
 word zotamt;
 byte e;
 void *mbrptr;
 
 mbrptr=&currentmbr;
 
 /* Reset disk */
 _asm xor ah, ah;
 _asm mov dl, 0x80;
 _asm int 0x13;
 _asm lahf;
 _asm mov e, ah;
 if (e&1) return;
 
 _asm mov ax, 0x0201;         /* read 1 sector */
 _asm mov cx, 0x0001;         /* cyl 0, sec 1 */
 _asm xor dh, dh;             /* head 0 */
 _asm mov dl, 0x80;
 _asm mov bx, mbrptr;
 _asm int 0x13;
 _asm lahf;
 _asm mov e, ah;
 if (e&1) return;
 
 /*
  * If there is a valid MBR there already, just replace the boot code.
  * Otherwise, zap the whole damn thing, then add the boot code.
  */
 if (currentmbr.signature!=0xAA55)
  zotamt=0x0200;
 else
  zotamt=0x01BE;
 
 memset(&currentmbr,0,zotamt);
 memcpy(&currentmbr,mbrboot,sizeof(mbrboot));
 currentmbr.signature=0xAA55;
 
 _asm mov ax, 0x0301;         /* write 1 sector */
 _asm mov cx, 0x0001;         /* cyl 0, sec 1 */
 _asm xor dh, dh;             /* head 0 */
 _asm mov dl, 0x80;
 _asm mov bx, mbrptr;
 _asm int 0x13;
}

int main (int argc, char **argv)
{
 byte t;
 nohd=nohds=must_reboot=0;
 
 if (argc==2)
 {
  if (argv[1][0]=='/')
  {
   if (!stricmp(argv[1], "/MBR"))
   {
    zot_mbr();
    return 0;
   }
   else if (!stricmp(argv[1], "/STATUS"))
   {
   }
  }
 }
 
 if (argc>1)
 {
  dos_puts(e_args);
  return 1;
 }
 
 for (t=0; t<128; t++)
 {
  byte d;
  
  d=t|128;
  _asm mov ah, 0x08;
  _asm mov dl, d;
  _asm xor di, di;
  _asm push es;
  _asm mov es, di;
  _asm int 0x13;
  _asm pop es;
  _asm lahf;
  _asm mov d, ah;
  if (!(d&1))
   hdtab[nohds++]=t|128;
 }
 
 if (!nohds)
 {
  dos_puts(e_nohds);
  return 1;
 }
 
 getmbr();
 dos_puts(e_banner);
 
 mainmenu();
 if (must_reboot) reboot();
 
 return 0;
}
