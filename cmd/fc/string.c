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

#include <string.h>

/*
 * Return first character in str that is also in set.
 * If none found, point to the NUL terminator.
 */
char *strbscan (char *str, char *set)
{
 char *p;
 
 if (!str) return 0; /* sanity */
 if (!set) return 0; /* sanity */
 for (p=str; *p; p++)
 {
  char *q;
  
  for (q=set; *q; q++)
   if (*p==*q) return p;
 }
 return p;
}

/*
 * Return first character in str that is not in set.
 * If all match, point to the NUL terminator.
 */
char *strbskip (char *str, char *set)
{
 char *p;
 
 if (!str) return 0; /* sanity */
 if (!set) return 0; /* sanity */
 for (p=str; *p; p++)
 {
  char *q;
  int dirty;

  dirty=0;
  for (q=set; *q; q++)
   if (*p==*q) dirty=1;
  
  if (!dirty) return p;
 }
 return p;
}

/*
 * Smash case, and return whether pref matches the beginning of str.
 * return -1 for match, 0 for no match.
 */
char strpre (char *pref, char *str)
{
 extern unsigned char toupper (unsigned char);
 
 unsigned char c1, c2;
 int l;
 
 l=strlen(pref);

 while (l--, (c1=toupper(*pref++))==(c2 = toupper(*str++)))
 {
  if (!c1) return 0;
 }
 
 return -1;
}
