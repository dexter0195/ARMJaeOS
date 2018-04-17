 # A didactic simulation of an arm OS running on the uarm emulator.
 # Copyright (C) 2016 Carlo De Pieri, Alessio Koci, Gianmaria Pedrini,
 # Alessio Trivisonno
 #
 # This program is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program.  If not, see <http://www.gnu.org/licenses/>.
 #
all: phase0 phase1 phase2

COMPILER ?= gcc
ARM_COMPILER ?= arm-none-eabi-gcc
ARM_LINKER ?= arm-none-eabi-gcc
ELF_SCRIPT ?= elf2uarm
ELF_FLAGS ?= -k
ULIBS ?= /usr/include/uarm
#std=gnu99 is needed since debian repo has an old version of arm-none-eabi-gcc
COMPILE_FLAGS ?= -std=gnu99 -I $(ULIBS) -I $(INCDIR) -I $(LIBSDIR) -c
ARM_COMPILE_FLAGS ?= -mcpu=arm7tdmi
ARM_LINKER_FLAGS ?= -nostartfiles -T $(ULIBS)/ldscripts/elf32ltsarm.h.uarmcore.x
LINK_ARM = $(ARM_LINKER) $(ARM_LINKER_FLAGS)
COMPILE_x86 = $(COMPILER) $(COMPILE_FLAGS)
COMPILE_ARM = $(ARM_COMPILER) $(COMPILE_FLAGS) $(ARM_COMPILE_FLAGS)
BINDIR ?= bin
SRCDIR ?= src
TESTDIR = $(SRCDIR)/test
LIBSDIR = $(SRCDIR)/libs
INCDIR = $(SRCDIR)/include
UARM_BIN ?= /usr/bin/uarm
UARM_CONF_PATH ?= uarm_conf
UARM_CONF2_PATH ?= uarm_conf2
UARM_FLAGS ?= -e -c $(UARM_CONF_PATH)
UARM_FLAGS2 ?= -e -c $(UARM_CONF2_PATH)
UARM_FLAGS2_DEBUG ?= -e -c $(UARM_CONF2_PATH)
UARM_EXEC = $(UARM_BIN) $(UARM_FLAGS)
UARM_EXEC2 = $(UARM_BIN) $(UARM_FLAGS2)
UARM_EXEC2_DEBUG = $(UARM_BIN) $(UARM_FLAGS2_DEBUG)


preliminary:
	if [ ! -d "$(BINDIR)" ]; then \
		mkdir "$(BINDIR)"; \
	fi

run0: phase0
	./$(BINDIR)/phase0

phase0: preliminary p0test.o
	cd $(BINDIR); \
	$(COMPILER) -o phase0 p0test.o

p0test.o: $(TESTDIR)/p0test.c $(INCDIR)/clist.h
	$(COMPILE_x86) -Dphase0 -o $(BINDIR)/p0test.o $(TESTDIR)/p0test.c

run1: phase1
	$(UARM_EXEC)

phase1: preliminary phase1.core.uarm

phase1.core.uarm: phase1.elf
	$(ELF_SCRIPT) $(ELF_FLAGS) $(BINDIR)/phase1.elf

phase1.elf: p1test.o pcb.o asl.o helplib.o
	$(LINK_ARM) -o $(BINDIR)/phase1.elf \
		$(ULIBS)/crtso.o $(ULIBS)/libuarm.o $(BINDIR)/p1test.o \
		$(BINDIR)/pcb.o $(BINDIR)/asl.o $(BINDIR)/helplib.o

p1test.o: $(TESTDIR)/p1test.c $(INCDIR)/const.h $(INCDIR)/clist.h $(INCDIR)/pcb.h $(INCDIR)/asl.h
	$(COMPILE_ARM) -o $(BINDIR)/p1test.o $(TESTDIR)/p1test.c

pcb.o: $(LIBSDIR)/pcb.c $(INCDIR)/const.h $(INCDIR)/pcb.h \
	$(INCDIR)/types.h $(INCDIR)/helplib.h $(INCDIR)/clist.h
	$(COMPILE_ARM) -o $(BINDIR)/pcb.o $(LIBSDIR)/pcb.c

asl.o: $(LIBSDIR)/asl.c $(INCDIR)/const.h $(INCDIR)/pcb.h \
	$(INCDIR)/types.h $(INCDIR)/asl.h $(INCDIR)/clist.h
	$(COMPILE_ARM) -o $(BINDIR)/asl.o $(LIBSDIR)/asl.c

helplib.o: $(LIBSDIR)/helplib.c $(INCDIR)/helplib.h $(INCDIR)/types.h
	$(COMPILE_ARM) -o $(BINDIR)/helplib.o $(LIBSDIR)/helplib.c

run2: phase2
	$(UARM_EXEC2)
rundebug2: debugphase2
	$(UARM_EXEC2_DEBUG)

phase2: preliminary phase2.core.uarm

phase2.core.uarm: phase2.elf
	$(ELF_SCRIPT) $(ELF_FLAGS) $(BINDIR)/phase2.elf

phase2.elf: p2test.o pcb.o asl.o helplib.o initial.o exceptions.o interrupts.o scheduler.o
	$(LINK_ARM) -o $(BINDIR)/phase2.elf \
		$(ULIBS)/crtso.o $(ULIBS)/libuarm.o $(BINDIR)/p2test.o \
		$(BINDIR)/pcb.o $(BINDIR)/asl.o $(BINDIR)/helplib.o \
		$(BINDIR)/initial.o $(BINDIR)/exceptions.o $(BINDIR)/interrupts.o $(BINDIR)/scheduler.o \
		$(DEBUG)

initial.o: $(SRCDIR)/initial.c $(INCDIR)/*
	$(COMPILE_ARM) -o $(BINDIR)/initial.o $(SRCDIR)/initial.c

exceptions.o: $(SRCDIR)/exceptions.c $(INCDIR)/*
	$(COMPILE_ARM) -o $(BINDIR)/exceptions.o $(SRCDIR)/exceptions.c

debugphase2: debug.o
	make phase2 COMPILE_FLAGS='$(COMPILE_FLAGS) -DDEBUG' DEBUG="$(BINDIR)/debug.o" ARM_COMPILE_FLAGS="$(ARM_COMPILE_FLAGS) -I $(TESTDIR)"

debug.o: $(TESTDIR)/*
	$(COMPILE_ARM) -I $(TESTDIR) -o $(BINDIR)/debug.o $(TESTDIR)/debug.c

interrupts.o: $(SRCDIR)/interrupts.c $(INCDIR)/* 
	$(COMPILE_ARM) -o $(BINDIR)/interrupts.o $(SRCDIR)/interrupts.c

scheduler.o: $(SRCDIR)/scheduler.c $(INCDIR)/*
	$(COMPILE_ARM) -o $(BINDIR)/scheduler.o $(SRCDIR)/scheduler.c

p2test.o: $(TESTDIR)/p2test.c $(INCDIR)/*
	$(COMPILE_ARM) -o $(BINDIR)/p2test.o $(TESTDIR)/p2test.c

clean:
	cd $(BINDIR); \
	rm -f phase1.elf p1test.o pcb.o asl.o helplib.o p0test.o \
		phase0 phase1.elf.core.uarm phase1.elf.stab.uarm \
		initial.o exceptions.o interrupts.o scheduler.o p2test.o \
		phase2.elf.core.uarm phase2.elf.stab.uarm phase2.elf debug.o

