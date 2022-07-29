John Elliott fixed a bug involving creating defective PSPs where the 
CP/M-style CALL 5 pointer is off by 2 bytes.

klys got this working in WASM, removing the dependency on non-free tools.

S. V. Nickolas began the process of updating it to support features added in
later versions of DEBUG.

GNU make is assumed, the makefile assumes the practice of Linux Watcom to use
.o rather than .obj for object files.  Note: you should make clean before
switching between the regular makefile and Makefile.ext.

Building with Makefile.ext will add the online help (/? and the ? command) as
found in MS-DOS and PC DOS 5 or later.

Features still missing: read/write on large partitions; EMS commands (the help
is present but commented out).

AFAICT, there isn't support for 186, 286, 386 etc. opcodes in the MS-DOS 5+
DEBUG, so I didn't make an effort to try to add it here either.
