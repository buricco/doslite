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

#ifndef _H_EXTERN_
#define _H_EXTERN_
#include <stdint.h>

#define HELP_VERSION "0.05"

#ifndef HELPFILE
#define HELPFILE "help.htx"
#endif

#ifndef HELPTOC
#define HELPTOC  "cmdcontents"
#endif

#define LNBUFSZ  256
#define PGBUFSZ  32768
#define MAXLINS  1024

typedef struct mos_stat {
 uint16_t btnmask;
 uint16_t x;
 uint16_t y;
} MOS_STAT;

#define MOS_LEFT   0x01
#define MOS_RIGHT  0x02
#define MOS_CENTER 0x04

int       mos_check    (void);
void      mos_csron    (void);
void      mos_csroff   (void);
void      mos_read     (MOS_STAT *p);
void      mos_move     (uint16_t x, uint16_t y);
uint8_t   vid_setmode  (uint8_t mode);
void      vid_gotoxy   (uint8_t x, uint8_t y);
void      vid_wrstrat  (uint8_t *s, uint8_t x, uint8_t y, int c);
void      vid_wrchrat  (uint8_t k, uint8_t x, uint8_t y, uint8_t c);
uint16_t  key_read     (void);
int32_t   key_poll     (void);

extern int gotmouse;
extern uint16_t vidseg;

#endif /* _H_EXTERN_ */

