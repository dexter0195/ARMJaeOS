/* debug.h is a small library to help debug our code.
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
#ifndef DEBUG_H
#define DEBUG_H
#include <libuarm.h>

/* Using bk will call one of six available function (zbk0-5). They do nothing at all,
 * but can be easly used as breakpoint. bk will also set the corresponding zdebug0-5
 * variable to the given value.
 * Typical usage would contemplate multiple calls of bk in a function on the same 
 * 'channel' with incremental 'value' to follow that function progression, while 
 * other calls to bk on different channels could follow a parent, or a wholly different
 * function.
 */
#define bk(channel,value)                       \
    do {                                        \
        switch (channel) {                      \
        case 0: {                               \
            zdebug0 = value;                    \
            zbk0();                             \
            break;                              \
        }                                       \
        case 1: {                               \
            zdebug1 = value;                    \
            zbk1();                             \
            break;                              \
        }                                       \
        case 2: {                               \
            zdebug2 = value;                    \
            zbk2();                             \
            break;                              \
        }                                       \
        case 3: {                               \
            zdebug3 = value;                    \
            zbk3();                             \
            break;                              \
        }                                       \
        case 4: {                               \
            zdebug4 = value;                    \
            zbk4();                             \
            break;                              \
        }                                       \
        case 5: {                               \
            zdebug5 = value;                    \
            zbk5();                             \
            break;                              \
        }                                       \
        default: break;}}while(0)

/* Call a dummy function that can be used as breakpoint and print something in the
 * terminal.
 * 
 * IMPORTANT NOTE: this may be more trouble than it's worth, as tprint() will work only
 * where interrupts are masked or if terminal interrupts are correctly handled. bk is
 * more reliable and flexible, use that one if you can.
 */
#define pbk(msg) ({  \
        tprint(msg); \
        zbk();        \
        })

/* This macro will replace the assembly routine call of uarm if debug.h is included last. Before
 * calling the original routine it will save into panic_FILE and panic_LINE the filename and
 * the line which originated the PANIC call. It will then try to tprint into the terminal
 * (see pbk documentation above about why this is not really safe).
 */
#define PANIC() \
    do { \
    panic_FILE = __FILE__; \
    panic_LINE=__LINE__; \
    tprint("\n Panic called in "); \
    tprint(__FILE__); \
    tprint(" at line "); \
    char* result = itoa(__LINE__, result, 10); \
    tprint(result); \
    tprint("\n\n"); \
    PANIC(); \
    }while(0)

void zbk();
void zbk0();
void zbk1();
void zbk2();
void zbk3();
void zbk4();
void zbk5();
int zdebug0;
int zdebug1;
int zdebug2;
int zdebug3;
int zdebug4;
int zdebug5;
char* itoa(int value, char* result, int base);  

// NOTE: this is a pointer, take care when trying to read from it in uarm! :)
char* panic_FILE;
int panic_LINE;
#endif /* end of include guard: DEBUG_H */
