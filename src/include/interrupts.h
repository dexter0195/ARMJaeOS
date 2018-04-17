/* This file manages the os interrupts and its timers
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
#ifndef _INTERRUPTS 
#define _INTERRUPTS
#include <types.h>

/* System interrupt handler function
 * We handle an interrupt at a time, the higher priority first */
void Interrupt_Handler();

/* Manage terminal devices */
void terminal_handler();

/* Find out the first device requesting an interrutp on a given line */
int which_device_on_line(int which_int);

/* Manage all devices but terminals */
void generic_device_handler(int which_int);

/* Check if the process which raised an interrupt and was blocked on the
 * corresponding real device semaphore has been killed */
bool is_process_killed(int which_int);

/* This function manages two system timers:
 *  - time slice: this is currently about 5ms, we assure every process to get its whole timeslice
 *      even if it has been interrupted
 *  - pseudo-clock timer: this is currently about 100ms. Due to several factors, we can't be sure
 *      about delays, but they don't add up since we use timestamps. */
int manage_timers();

// These are used by the syscall handler and manage_timers to decide what to do next
#define INT_PSEUDO_CLOCK_ENDED 0
#define INT_TIME_SLICE_ENDED 1
#define PROCESSOR_TWIDDLING_ITS_THUMBS 42
#endif
