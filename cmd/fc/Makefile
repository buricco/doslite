# Copyright 2024 S. V. Nickolas.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the Software), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT,TORT OR OTHERWISE, ARISING FROM,
# OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

fc.exe:	fc.obj fgetl.obj ntoi.obj string.obj move.obj messages.obj error.obj update.obj
	wcl -fe=fc.exe fc.obj fgetl.obj ntoi.obj string.obj move.obj messages.obj error.obj update.obj

fc.obj:	fc.c fc.h tools.h ttypes.h
	wcl -fo=fc.obj -c -D__WATCOM__ fc.c

fgetl.obj:	fgetl.c tools.h ttypes.h
	wcl -fo=fgetl.obj -c -D__WATCOM__ fgetl.c

ntoi.obj:	ntoi.c tools.h ttypes.h
	wcl -fo=ntoi.obj -c -D__WATCOM__ ntoi.c

string.obj:	string.c
	wcl -fo=string.obj -c -D__WATCOM__ string.c

move.obj:	move.c
	wcl -fo=move.obj -c -D__WATCOM__ move.c

messages.obj:	messages.c
	wcl -fo=messages.obj -c -D__WATCOM__ messages.c

error.obj:	error.c tools.h ttypes.h
	wcl -fo=error.obj -c -D__WATCOM__ error.c

update.obj:	update.c tools.h ttypes.h									
	wcl -fo=update.obj -c -D__WATCOM__ update.c

clean:
	rm -f fc.exe fc.obj fgetl.obj ntoi.obj string.obj move.obj messages.obj error.obj update.obj
