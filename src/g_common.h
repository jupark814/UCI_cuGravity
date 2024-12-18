/**
 * g_common.h
 * Copyright (C) Tony Givargis, 2019-2020
 *
 * This file is part of The Gravity Compiler.
 *
 * The Gravity Compiler is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version. The Gravity Compiler is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details. You should
 * have received a copy of the GNU General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _G_COMMON_H_
#define _G_COMMON_H_

#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#define G__VERSION 10

#define G__ERR_MEMORY   -100
#define G__ERR_SYSTEM   -101
#define G__ERR_ARGUMENT -102
#define G__ERR_SOFTWARE -103
#define G__ERR_SYNTAX   -104
#define G__ERR_FILE     -105
#define G__ERR_JITC     -106

#define G__MIN(a,b) ((a) < (b) ? (a) : (b))
#define G__MAX(a,b) ((a) > (b) ? (a) : (b))

#define G__UNUSED(x) do {			\
		(void)(x);			\
	} while (0)

#define G__DEBUG(e) do {			\
		if (g__debug_enabled) {		\
			fprintf(stderr,		\
				"error:"	\
				" %s:%d: %s\n",	\
				__FILE__,	\
				__LINE__,	\
				g__error(e));	\
		}				\
	} while (0)

#define G__FREE(m) do {				\
		if (m) {			\
			free((void *)(m));	\
			(m) = 0;		\
		}				\
	} while (0)

extern int g__debug_enabled;

void g__free(void *p);

void g__sprintf(char *str, size_t size, const char *format, ...);

void *g__malloc(size_t n);

const char *g__strdup(const char *s);

const char *g__error(int e);

void g__unlink(const char *pathname);

__attribute__((__unused__)) static size_t
g__strlen(const char *s)
{
	return s ? strlen(s) : 0;
}

#endif /* _G_COMMON_H_ */
