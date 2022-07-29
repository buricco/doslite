Copyright 2022 S. V. Nickolas.

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  

These programs are untested, but aim to implement various pieces of the MODE
command.  The concept is to implement all the various pieces of MODE and then
combine them all into a single tool.  Basically, divide and conquer.

There are a lot of different features that MODE has to support.  Some of them
even require going TSR.  These are going to be harder to implement in C, but
the other elements are harder to implement in ASM, so you win some, you lose
some.

MORE was simpler in PC DOS before version 4, but it was still insanely complex
even as early as version 2, so this is a nightmare to code.
