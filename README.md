Introduction
-------
JaeOS is a Kernel for the uARM architecture.
It implements all the basic functionalities of a kernel such as system initialization, scheduling, syscall handling,
interrupt handling (clock and devices) and  multiprocess syncronization (with semaphores).
All the kernel code and libraries are written from scratch (no stdlib) and only a basic bios was given.
The project also proves the ability to learn and follow documentation (both the chipset and the project one).

This project was done by the group lso16az07 as a part of the Operating System course (2015-2016) held by Professor Renzo Davoli at University of Bologna.

License
-------
This software is released under the GPL v3.0 licence.

Members
----------
 - Carlo De Pieri carlo.depieri@studio.unibo.it
 - Alessio Koci alessio.koci@studio.unibo.it
 - Gianmaria Pedrini [me] gianmaria.pedrini@studio.unibo.it (gianmariapedrini@gmail.com)
 - Alessio Trivisonno  alessio.trivisonno@studio.unibo.it

Usage
--------
1. compile everything or only a specific phase
    - make
    - make phase0
    - make phase1
    - make phase2
2. manually test clist.h (requested library in phase 0)
    - ./bin/phase0 OR
    - make run0
3. test phase1
    - run uarm and load bin/phase1.elf.core.uarm and bin/phase1.elf.stab.uarm OR
    - make run1
4. test phase2
    - run uarm and load bin/phase2.elf.core.uarm and bin/phase2.elf.stab.uarm OR
    - make run2

Debug
-----
A small libray is available to help debug phase2 code. To use it, include debug.h as
the LAST header (so it can override some functions/macros). Remember to compile using:
 - make debugphase2 OR 
 - make rundebug2
More information about this library can be found reading the source file.

Compile options
---------------
During compilation uarm libraries are needed. Make will look them up into /usr/include/uarm,
but a custom path can be used instead with 'make ULIBS=/path/to/my/libs'.

Optionally, things like compilers and linkers flags, source and bin folders, uarm config
files can be passed to make. See the Makefile for details.

NOTE: If you change bin folder, take care of the relative field in uarm_conf as well.

Compilation and execution was tested under:
 - 3.16.0-38-generic 14.04 Ubuntu x86\_64 GNU/Linux with gcc v4.8.4
 - 4.3.0-1-amd64 Debian 4.3.3-7 x86\_64 GNU/Linux with gcc v5.3.1
 - 4.2.0-1-amd64 Debian 4.2.6-3 x86\_64 GNU/Linux with gcc v5.3.1
 - 3.16.0-4-amd64 Debian 3.16.36-1 x86\_64 gcc 4.9.2 and arm-none-eabi-gcc 4.8.4
 - 4.7.2-1-ARCH x86\_64 GNU/Linux with gcc v6.2.1 and arm-none-eabi-gcc v6.2.0
 - 4.7.1-1-ARCH x86\_64 GNU/Linux with gcc v6.1.1 and arm-none-eabi-gcc v6.1.0
