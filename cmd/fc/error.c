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

/* error.c - return text of error corresponding to the most recent DOS error */

#ifdef __WATCOM__
#include <errno.h>
#include <string.h>

char *error (void)
{
 return strerror(errno);
}
#else
#include "tools.h"

extern int errno;
extern sys_nerr;
extern char *sys_errlist[];
extern char UnKnown[];

char *error (void)
{
    if (errno < 0 || errno >= sys_nerr)
	return UnKnown;
    else
	return sys_errlist[errno];
}
#endif
