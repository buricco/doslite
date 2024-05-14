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

unsigned char 
 BadSw[]={"Incompatible switches"},
 BadOpn[]={"Cannot open %s - %s"},
 LngFil[]={"%s longer than %s"},
 NoDif[]={"No differences encountered"},
 NoMem[]={"Out of memory\n"},
 ReSyncMes[]={"Resync failed.  Files are too different\n"},
 e_switch[]={"Invalid switch"},
 e_noarg[]={"Required parameter missing"},
 e_xsarg[]={"Too many parameters"},
 UnKnown[]={"Unknown error"};

#ifdef HELP
 unsigned char m_help[]={
"Compares two files or sets of files and displays the differences between them.\n"
"\n"
"FC [/A] [/C] [/L] [/LBn] [/N] [/T] [/W] [/nnnn] [drive1:][path1]filename1\n"
"  [drive2:][path2]filename2\n"
"FC /B [drive1:][path1]filename1 [drive2:][path2]filename2\n"
"\n"
"  /A     Displays only first and last lines for each set of differences.\n"
"  /B     Performs a binary comparison.\n"
"  /C     Disregards the case of letters.\n"
"  /L     Compares files as ASCII text.\n"
"  /LBn   Sets the maximum consecutive mismatches to the specified number of\n"
"         lines.\n"
"  /N     Displays the line numbers on an ASCII comparison.\n"
"  /T     Does not expand tabs to spaces.\n"
"  /W     Compresses white space (tabs and spaces) for comparison.\n"
"  /nnnn  Specifies the number of consecutive lines that must match after a\n"
"         mismatch.\n"};
#endif
