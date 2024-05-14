/*
 * Copyright 1983-1988
 *     International Business Machines Corp. amd Microsoft Corp.
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
 * THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* fgetl.c - expand tabs and return lines w/o separators */

#include "tools.h"

static int min (int a, int b) {return (a<b)?a:b;}

/* returns line from file (no CRLFs); returns NULL if EOF */
int fgetl (char *buf, int len, FILE *fh)
{
    register int c;
    register char *p;

    /* remember NUL at end */
    len--;
    p = buf;
    while (len) {
	c = getc (fh);
	if (c == EOF || c == '\n')
	    break;
#if MSDOS
	if (c != '\r')
#endif
	    if (c != '\t') {
		*p++ = c;
		len--;
		}
	    else {
		c = min (8 - ((p-buf) & 0x0007), len);
		Fill (p, ' ', c);
		p += c;
		len -= c;
		}
	}
    *p = 0;
    return ! ( (c == EOF) && (p == buf) );
}

/* writes a line to file (with trailing CRLFs) from buf, return <> 0 if
 * writes fail
 */
int fputl (char *buf, int len, FILE *fh)
{
#if MSDOS
    return (fwrite (buf, 1, len, fh) != len || fputs ("\r\n", fh) == EOF) ? EOF : 0;
#else
    return (fwrite (buf, 1, len, fh) != len || fputs ("\n", fh) == EOF) ? EOF : 0;
#endif
}
