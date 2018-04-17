/* This is a small library to help debug our code. More info in the header!
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
#include <debug.h>

void zbk(){}
void zbk0(){}
void zbk1(){}
void zbk2(){}
void zbk3(){}
void zbk4(){}
void zbk5(){}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3. */
char* itoa(int value, char* result, int base)
{
    // check that the base if valid
    if ( base < 2 || base > 36 ) {
	*result = '\0';
	return result;
    }
    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;
    do {
	tmp_value = value;
	value /= base;
	*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while ( value );
    // Apply negative sign
    if ( tmp_value < 0 )
	*ptr++ = '-';
    *ptr-- = '\0';
    while ( ptr1 < ptr ) {
	tmp_char = *ptr;
	*ptr-- = *ptr1;
	*ptr1++ = tmp_char;
    }
    return result;
}
