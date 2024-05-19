/*
 * Copyright 2024 S. V. Nickolas.
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
 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
 * This code is far from complete.
 * Stubs for some of the TUI elements are present, and the first page of the
 * topic is displayed.  The rest of the interface is not implemented, and in
 * fact a lot of work is still necessary to make that even possible.
 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
 */

/*
 * Specification (some of these may require significant reworking to support):
 * 
 *   HELP [/B] [/G] [/H] [/NOHI] [topic]
 * 
 *     /B - use colors suited to a monochrome display hooked to a CGA
 *     /G - don't care about the possibility of "snow" on genuine CGAs
 *     /H - set 43-line mode on EGA, 50-line mode on VGA
 *     /NOHI - change the bold color from bright white to dark cyan
 *     topic - initial topic search term; default is "cmdcontents"
 * 
 * There is some information at https://github.com/fancidev/DosHelp relevant
 * to the compression format, but I haven't processed it yet.
 * 
 * Current differences from the MS-DOS 6 version:
 * 
 *   1. No use of QBASIC and the user interface code is bespoke.  I want it to
 *      look as close to QBASIC's COW library as possible, but I'd like it to
 *      fit in a COM file if possible.
 *   2. Allergy to the term "MS-DOS" (for obvious reasons); "MS-DOS Help" is
 *      replaced by "Help" in most instances, "PC DOS/RE Help Viewer" in the
 *      About box, and is completely removed from the window title.
 * 
 * It will of course be necessary to rewrite the help compressor (HELPMAKE),
 * as we don't have the rights to it!  This is true even though we will not be
 * shipping it with the PC DOS/RE system.
 * 
 * I *could* write this with Microsoft C 5.1 in mind, but that compiler is too
 * braindead and I'd rather use Watcom.
 */

/*
 * UI status:
 * 
 *   * About dialog: Done with bugs
 *   * Error dialog: Done with bugs
 *   * Print dialog: Nothing
 *   * Printer setup dialog: Nothing
 *   * Find dialog: Nothing
 *   * Find engine: Nothing
 *   * Menu subsystem: Nothing
 *   * Mouse stuff: Nonfunctional
 *   * Main window: Started, but nowhere near done
 *   * Huffman codec: Nothing
 * 
 * *glub* ...help?
 */

#include <dos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extern.h"
#include "messages.h"

char linbuf[LNBUFSZ+10];
char inlin[LNBUFSZ];
char pgttl[LNBUFSZ];
char *pgbuf;
uint16_t lintab[MAXLINS];
uint16_t linct;
FILE *file;

/*
 * Display about dialog.
 * XXX: mouse no wrk.
 */
void do_about (void)
{
 uint8_t membak[1600];
 uint16_t a;
 uint8_t far *p;
 uint8_t x, y, w, c, okl, okr;
 
 p=MK_FP(vidseg, 7*160);
 for (a=0; a<(10*160); a++) membak[a]=p[a];
 
 vid_wrstrat(w_alt,1,24,0x70);
 
 w=strlen(d_about1);
 if (strlen(d_about2)>w) w=strlen(d_about2);
 if (strlen(d_about3)>w) w=strlen(d_about3);
 w+=6;
 vid_wrchrat(218,40-(w>>1),8,0x70);
 vid_wrchrat(191,40+(w>>1),8,0x70);
 vid_wrchrat(192,40-(w>>1),16,0x70);
 vid_wrchrat(217,40+(w>>1),16,0x70);
 for (y=9; y<16; y++)
 {
  vid_wrchrat(179,40-(w>>1),y,0x70);
  vid_wrchrat(179,40+(w>>1),y,0x70);
  for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(' ',x,y,0x70);
 }
 for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(196,x,8,0x70);
 for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(196,x,14,0x70);
 for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(196,x,16,0x70);
 vid_wrchrat(195,40-(w>>1),14,0x70);
 vid_wrchrat(180,40+(w>>1),14,0x70);
 okl=37-(strlen(d_ok)>>1);
 okr=42+(strlen(d_ok)>>1);
 vid_wrstrat(d_ok, 40-(strlen(d_ok)>>1),15,0x70);
 vid_wrchrat('<', okl,15,0x7F);
 vid_wrchrat('>', okr,15,0x7F);
 vid_wrstrat(d_about1, 40-(strlen(d_about1)>>1), 10, 0x70);
 vid_wrstrat(d_about2, 40-(strlen(d_about2)>>1), 11, 0x70);
 vid_wrstrat(d_about3, 40-(strlen(d_about3)>>1), 12, 0x70);
 vid_gotoxy(40-(strlen(d_ok)>>1),15);
 
 while (1)
 {
  int k;
  MOS_STAT m;
  
  if (gotmouse)
  {
   mos_read(&m);
   if ((m.btnmask&MOS_LEFT)&&(m.y==15)&&(m.x>=okl)&&(m.x<=okr)) break;
  }
  
  if (key_poll())
  {
   k=key_read();
   /* if ((k==' ')||(k=='\r')) */ break;
  }
 }
 
 for (a=0; a<(10*160); a++) p[a]=membak[a];
 for (x=0; x<80; x++) vid_wrchrat(' ',x,24,0x70);
}

/*
 * Shares most of its code with do_about().
 * Make changes to both, generally.
 */
void do_alert (char *msg)
{
 uint8_t membak[1280];
 uint16_t a;
 uint8_t far *p;
 uint8_t x, y, w, c, okl, okr;
 
 p=MK_FP(vidseg, 8*160);
 for (a=0; a<(8*160); a++) membak[a]=p[a];
 
 vid_wrstrat(w_alt,1,24,0x70);
 
 w=strlen(msg);
 w+=6;
 vid_wrchrat(218,40-(w>>1),9,0x70);
 vid_wrchrat(191,40+(w>>1),9,0x70);
 vid_wrchrat(192,40-(w>>1),15,0x70);
 vid_wrchrat(217,40+(w>>1),15,0x70);
 for (y=10; y<15; y++)
 {
  vid_wrchrat(179,40-(w>>1),y,0x70);
  vid_wrchrat(179,40+(w>>1),y,0x70);
  for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(' ',x,y,0x70);
 }
 for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(196,x,9,0x70);
 for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(196,x,13,0x70);
 for (x=41-(w>>1); x<40+(w>>1); x++) vid_wrchrat(196,x,15,0x70);
 vid_wrchrat(195,40-(w>>1),13,0x70);
 vid_wrchrat(180,40+(w>>1),13,0x70);
 okl=37-(strlen(d_ok)>>1);
 okr=42+(strlen(d_ok)>>1);
 vid_wrstrat(d_ok, 40-(strlen(d_ok)>>1),14,0x70);
 vid_wrchrat('<', okl,14,0x7F);
 vid_wrchrat('>', okr,14,0x7F);
 vid_wrstrat(msg, 40-(strlen(d_about2)>>1), 11, 0x70);
 vid_gotoxy(40-(strlen(d_ok)>>1),14);
 
 while (1)
 {
  int k;
  MOS_STAT m;
  
  if (gotmouse)
  {
   mos_read(&m);
   if ((m.btnmask&MOS_LEFT)&&(m.y==14)&&(m.x>=okl)&&(m.x<=okr)) break;
  }
  
  if (key_poll())
  {
   k=key_read();
   /* if ((k==' ')||(k=='\r')) */ break;
  }
 }
 
 for (a=0; a<(8*160); a++) p[a]=membak[a];
 for (x=0; x<80; x++) vid_wrchrat(' ',x,24,0x70);
}

int gettopic (char *topic, int smash)
{
 uint16_t t;
 char *ptr;
 
 sprintf (linbuf, ".context %s", topic);
 fseek (file, 0, SEEK_SET);
 while (1)
 {
  fgets (inlin, LNBUFSZ-2, file);
  if (feof(file)) return -1;
  if (inlin[strlen(inlin)-1]=='\n') inlin[strlen(inlin)-1]=0;
  if (inlin[strlen(inlin)-1]=='\r') inlin[strlen(inlin)-1]=0;
  if (!(smash?strcasecmp:strcmp)(inlin, linbuf)) break;
 }
 while (1)
 {
  fgets (inlin, LNBUFSZ-2, file);
  if (feof(file)) return -1;
  if (*inlin!='.') break;
 }
 if (inlin[strlen(inlin)-1]=='\n') inlin[strlen(inlin)-1]=0;
 if (inlin[strlen(inlin)-1]=='\r') inlin[strlen(inlin)-1]=0;
 if (*inlin!=':') return -1;
 if (inlin[1]!='n') return -1;
 strcpy(pgttl, &(inlin[2]));

 memset(pgbuf, 0, PGBUFSZ);
 
 linct=0;
 ptr=pgbuf;
 while (1)
 {
  fgets (inlin, LNBUFSZ-2, file);
  if (feof(file)) break;
  if (*inlin==':') continue; /* ? */
  if (!strncmp(inlin, ".context ", 9)) break;
  lintab[linct++]=strlen(pgbuf);
  if (inlin[strlen(inlin)-1]=='\n') inlin[strlen(inlin)-1]=0;
  if (inlin[strlen(inlin)-1]=='\r') inlin[strlen(inlin)-1]=0;
  if (ptr!=pgbuf) *(ptr++)='\n';
  strcat(ptr, inlin);
  ptr+=strlen(inlin);
 }
 
 /* Change newlines to terminators */
 for (t=0; t<linct; t++)
 {
  if (lintab[t])
  {
   pgbuf[lintab[t]++]=0;
  }
 }
 return 0;
}

void frame (char *title)
{
 uint8_t l, x, y;
 
 l=strlen(title)>>1;
 vid_wrchrat (218, 0, 1, 0x07);
 for (x=1; x<(40-l); x++) vid_wrchrat(196, x, 1, 0x07);
 vid_wrchrat(32, x++, 1, 0x70);
 vid_wrstrat(title, x, 1, 0x70);
 x+=strlen(title);
 vid_wrchrat(32, x++, 1, 0x70);
 for (; x<79; x++) vid_wrchrat(196, x, 1, 0x07);
 vid_wrchrat(191, x, 1, 0x07);
 for (y=2; y<23; y++)
 {
  vid_wrchrat(179, 0, y, 0x07);
  vid_wrchrat(176, 79, y, 0x07);
 }
 vid_wrchrat(24, 79, 2, 0x70);
 vid_wrchrat(25, 79, 22, 0x70);
 vid_wrchrat(192, 0, 23, 0x07);
 for (x=1; x<79; x++) vid_wrchrat(196, x, 23, 0x07);
 vid_wrchrat(217, 79, 23, 0x07);
}

void display (void)
{
 uint8_t cx;
 uint16_t cy;
 uint8_t ct;
 uint16_t t;
 
 ct=0;
 
 vid_gotoxy(0, 0);
 
 for (cx=0; cx<80; cx++)
 {
  vid_wrchrat(32, cx, 0, 0x70);
  vid_wrchrat(32, cx, 24, 0x70);
 }

 for (cy=2; cy<23; cy++)
  for (cx=1; cx<79; cx++)
   vid_wrchrat(32, cx, cy, 0x07);
 
 frame(pgttl);
 
 vid_wrstrat(w_status, 1, 24, 0x70);
 vid_wrstrat(w_file, 1, 0, -7);
 vid_wrstrat(w_search, 2+strlen(w_file), 0, -7);
 vid_wrstrat(w_help, 78-strlen(w_help), 0, -7);
 
 for (t=0; t<21; t++)
 {
  if (t<linct) vid_wrstrat(&(pgbuf[lintab[t+ct]]), 1, t+2, -1);
 }
 
//top:
 //vid_gotoxy(cx+1, 2+(cy-ct));
 vid_gotoxy(0,0);
 do_alert("This is a test.");
}

int do_help (char *inittopic, int smash)
{
 int e;
 uint8_t m;
 
 gotmouse=mos_check();
 
 if (strlen(inittopic)>LNBUFSZ)
 {
  fprintf (stderr, "%s\n", e_long);
  return -1;
 }
 
 pgbuf=malloc(PGBUFSZ);
 if (!pgbuf)
 {
  fprintf (stderr, "%s\n", e_ram);
  fclose(file);
  return -1;
 }

 file=fopen(HELPFILE, "rt");
 if (!file)
 {
  fprintf (stderr, "%s\n", e_file);
  fclose(file);
  free(pgbuf);
  return -1;
 }

 e=gettopic (inittopic, smash);
 if (e)
 {
  fprintf (stderr, "%s\n", e_404);
 }
 else
 {
  _asm {
   mov ah, 0x0F;
   int 0x10;
   mov m, al;
  }
  
  if (m==7)
  {
   vidseg=0xB000;
  }
  else
  {
   vidseg=0xB800;
   vid_setmode(3);
  }
  if (gotmouse) mos_csron();
  display();
  if (gotmouse) mos_csroff();
 }
 
 free(pgbuf);
 fclose(file);
 vid_setmode(m);
 return e;
}

int main (int argc, char **argv)
{
 switch (argc)
 {
  case 1:
   return do_help (HELPTOC, 0);
  case 2:
   return do_help (argv[1], 1);
  default:
   fprintf (stderr, "%s: usage: %s topic\n", argv[0], argv[0]);
   return 1;
 }
}
