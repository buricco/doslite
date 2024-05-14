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

/* convert an arbitrary based number to an integer */

#include <ctype.h>
#include "tools.h"

/* p points to characters, return -1 if no good characters found
 * and base is 2 <= base <= 16
 */
int ntoi (char *p, int base)
{
    register int i, c;
    flagType fFound;

    if (base < 2 || base > 16)
	return -1;
    i = 0;
    fFound = FALSE;
    while (c = *p++) {
	c = tolower (c);
	if (!isxdigit (c))
	    break;
	if (c <= '9')
	    c -= '0';
	else
	    c -= 'a'-10;
	if (c >= base)
	    break;
	i = i * base + c;
	fFound = TRUE;
	}
    if (fFound)
	return i;
    else
	return -1;
}
