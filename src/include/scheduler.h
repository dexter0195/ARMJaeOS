/* This is the os scheduler
 *
 * A didactic simulation of an arm OS running on the uarm emulator.
 * Copyright (C) 2016 Carlo De Pieri, Alessio Koci, Gianmaria Pedrini,
 * Alessio Trivisonno
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _SCHEDULE
#define _SCHEDULE
#include <types.h>

// These are used to give the scheduler information about where it has been called
#define SCHED_INIT 0
#define SCHED_TIME_SLICE_ENDED 1
#define SCHED_PROC_KILLED 2
#define SCHED_NEARWAIT 3
#define SCHED_PROC_BLOCKED 4

/* Main scheduler function. It's argument its used to take different action based on where the
 * scheduler is called from.
 * This is a simple round robin scheduler. */
void schedule(int state);
#endif
