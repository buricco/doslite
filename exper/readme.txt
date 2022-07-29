Experimental Folder
===================

  Here lie several unfinished implementations of MS-DOS utilities.  Some of
  them are barely stubs, and some of them are semi-functional.  Many of them
  are very buggy.
  
  In this file, I will attempt to explain the issues with each program.
  
  ANSI
  ----
  
    The TSR portion is waiting on the rest of it to be fixed.  A transient
    input filter version was written in order to test the parsing (e.g., with
    something like DOPEFISH.ANS from Commander Keen 4), but the output is very
    wrong.
    
    When the parsing is fixed, there is a note explaining how it could perhaps
    be converted into a proper ANSI.SYS driver.
    
    As an alternative, can Ziff-Davis be convinced to relicense their version?

  COMMAND
  -------
  
    You'll find a program in this folder that implements a lot of the internal
    commands from COMMAND.COM (it's woefully incomplete), but it lacks the
    shell, batch support, INT24 handling, etc., as well as a couple of the
    more complicated commands.
    
    DATE and TIME don't work right.
    
    This is a partial rewrite of RMF-COM, my previous command shell, which is
    some of my oldest surviving code.
  
  DEBUG
  -----
  
    This is actually mostly functional.  Anything from MS-DOS 2.11 should work
    just fine.  So what's the problem?
    
    There are two problems.
    
    The first is that sector loads and saves do not support BIGFAT partitions.
    (This is INT25 and INT26; information on what exactly the issue is should
    be easy to find on RBIL.)
    
    The second is that I have no idea how to implement the X commands, as they
    are implemented in MS-DOS 4 and later.
  
  FC
  --
  
    The binary compare portion of this is or should be complete.
    
    I have no idea how to implement the ASCII compare, which is comparable to
    "diff" on Unix.  (Note that if necessary, the MS-DOS 2.11 version of FC,
    written in assembler, and the Plan 9 from Bell Labs and Minix versions of
    diff, both written in C, are available and could possibly be used to aid
    in completing this project.)
    
  FDISK
  -----
  
    I've only just barely started on this one, and I'm in deep, since I have
    no reference to look at to see what I'm supposed to be doing.
    
    Ideally I'd like to replace the MBR code too; this is an adaptation of IBM
    black code that pretty much everyone bases their MBR on.
    
    This is one of three "breaker" utilities that must function properly
    before it makes sense to put DOSLITE into social version control, along
    with CHKDSK and FORMAT.
    
  FORMAT
  ------
  
    Deep in the dark, oh oh oh, the thunder in her heart, oh oh oh...
    
    Oh, right.  I managed to write the scaffolding, and then my head exploded.
    I was thinking of adapting Robert Nordier's FORMAT command, but his code
    is nigh unreadable.
    
    This is another "breaker" utility that I need working before I start
    uploading DOSLITE to social version control.

  MEM
  ---
  
    This is definitely not the MEM command you know from MS-DOS 6.  It is more
    like the version in PC DOS 4, with some of the functionality from MS-DOS
    5.  I really had to wrack my brain trying to get XMS calls working from C
    (as you're not supposed to be able to do this).
    
    Some of the functionality from the /PROGRAM and /DEBUG switches is
    unfinished.

    XMS3 functionality is not used so only 16 MB of XMS memory will be seen.
    
    I assume a 16K EMS page.  That might not always be correct.
    
    I do not currently know a reliable way to detect all extended memory as
    PC DOS 4 MEM shows it, i.e., all int15 memory before int15 is hooked by
    HIMEM.
  
  MOVE
  ----
  
    There's some bugs in this one, even though it's mostly finished.
    
    You'll find a good summary of what I'm having trouble with in the source.
  
  RESTORE
  -------
  
    It's half-written and nonfunctional, but it's at least a start.
    
  XCOPY
  -----
  
    This is just the scaffolding.  The actual bones of the program aren't
    there yet.  I'm having trouble wrapping my brain around how to implement
    it, especially the "buffering" system.
