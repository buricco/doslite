This is the current source tree for the DOSLITE Project.  It is a bit of a
mess, but it reflects the exact state of the source code at the time of the
upload.

DOSLITE aims to replace components of MS-DOS piece by piece, eventually to
include the kernel.  Although the DOSLITE components generally are meant to
implement the versions of commands from MS-DOS 6.21 or PC DOS 7.0, they are
also intended to be compatible with versions of either operating system down
to version 3.3 with minimal degradation of functionality.  Additionally, the
error responses have been made more internally consistent than MS-DOS/PC DOS.
More importantly, however, in line with the concept of software freedom,
DOSLITE is also made fully available to the power user to use, share, and
improve (in any combination) as the user pleases.

Generally, if a utility is complete, this means it will be fully functional at
the same level as MS-DOS 6.21 (or, if it does not exist on MS-DOS, PC DOS
7.0).  There are some cases where a PC DOS 7.0 version of a utility will have
functionality that the DOSLITE version does not.  Generally, if you know how
to use or have documentation for the MS-DOS version of the utility, it will
apply to DOSLITE.

Only with difficulty have I been able to create these tools.  In a few cases I
have availed myself (within the restrictions of the licenses) of some
third-party code, and sometimes studying it has allowed me to improve on my
own code.  I will accept help with tools I am struggling with, or with
suggestions to improve existing tools.  (Note that LFN support is NOT on the
table, as that introduces a whole can of worms regarding parser complexity and
having to support two completely different file APIs.)

If the reference to software freedom makes DOSLITE sound a lot like FreeDOS,
it is not entirely a coincidence.  Both operating systems are MS-DOS clones,
and both operating systems aim to maintain software freedom, but the
similarity ends there: where FreeDOS is the "GNU" of MS-DOS clones, DOSLITE
aims to be the "BSD".  As one example, FreeDOS applications are often bigger
but more powerful versions of those from MS-DOS, while their DOSLITE
counterparts aim to be exactly as powerful as those of MS-DOS and, often,
smaller.  "Codegolfing", or the process of whittling away code to produce the
smallest result, is not uncommon.

The "BSD" analogy regarding DOSLITE extends to the licensing.  If code is
given to me and I am unable to use it under terms comparable to the standard
BSD license (such as the modified UIUC license I use for most of my code, or
the MIT license used for the opened MS-DOS 2.11 source, some of which has
indeed been incorporated into DOSLITE), I will not incorporate it into the
project.

The "cmd" folder contains source code to commands I think are complete.  All
of these should contain makefiles coded for GNU make.

The "dev" folder contains source code to drivers or data files I think are
complete.  Again these should contain makefiles coded for GNU make.

The "exper" folder contains unfinished or experimental code which may range
anywhere from "it doesn't work" to "it's finished but suffers from some major
bugs".  Keep that in mind.  They may not have makefiles.  Generally if a
program is unfinished, something will say what the problem is - usually in the
source code.

Binaries of non-experimental utilities, drivers, data files, etc., can be
found in the "bin" folder.  These have not been run through any executable
packers, although I will probably run some of them through APACK for the
ultimate release.  These should be able to plug into an existing install of
MS-DOS or PC DOS (NOT Win9x, OS/2 or NT) with the proviso that PC DOS 7's
"FIND" command is more advanced than the one provided here.

Other folders may appear in later source bundles, as they become necessary.

If a utility does not appear here at all it may be that I have not been able
to or had the chance to implement it.

All utilities have been written to compile with either OpenWatcom C/C++ 1.9 or
the Netwide Assembler.  You may be able to compile C and ASM code with Borland
or Microsoft tools and A86 code with YASM, but this cannot be guaranteed.
