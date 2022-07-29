This is the EDLIN command from MS-DOS 2.11, converted minimally to build with
WASM instead of requiring MASM.  For the most part, only the necessary changes
have been made.

For compatibility with the MS-DOS 5 version of EDLIN, online help (through the
/? switch at startup, or the ? command at runtime) has been hacked in, using
the same messages as those versions.  This is only available if assembled with
the -DHELP switch.  (The binary here included contains these features.)

The makefiles, coded for GNU make, will roll EDLIN using the Watcom tools,
assuming the behavior of the Linux version which uses the ".o" extension for
object files rather than the ".obj" extension used on MS-DOS, Windows and
OS/2.  If switching from one makefile to the other, do a "make clean" first.
The default makefile will generate the no-frills build, while Makefile.ext
will generate one with the online help, as described above.
