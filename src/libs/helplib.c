/* A collection of helper functions to supply the lack of C stdlib
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

#include <types.h>

/* our implementation of the memset function offered by string.h
 * sets the object pointed by s to the value of c, byte by byte. */

void* mymemset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    // consider only the first byte of c if its larger than one byte (it shouldn't be)
    unsigned char tmp = c & 0xff;
    while (n--)
        *p++ = tmp;
    return s;
}

/* our implementation of the memcpy function offered by string.h
 * copies n bytes from the object pointed by "from" to the object pointed by "to", byte by byte. */

void mymemcopy(void* from, void* to, size_t n){
    char *cfrom = (char *)from;
    char *cto = (char *)to;
    for (int i=0; i<n; i++)
        cto[i] = cfrom[i];
}
