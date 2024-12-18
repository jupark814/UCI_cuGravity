/**
 * g_common.c
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

#define _GNU_SOURCE

#include <unistd.h>
#include "g_common.h"

int g__debug_enabled;

void
g__sprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;

	assert( str && size && format );

	va_start(ap, format);
	if (size <= (size_t)vsnprintf(str, size, format, ap)) {
		G__DEBUG(G__ERR_SOFTWARE);
		assert( 0 );
		exit(-1);
	}
	va_end(ap);
}

void *
g__malloc(size_t n)
{
	void *p;

	p = malloc(n);
	if (!p) {
		G__DEBUG(G__ERR_MEMORY);
		return 0;
	}
	return p;
}

const char *
g__strdup(const char *s_)
{
	char *s;

	s = g__malloc(g__strlen(s_) + 1);
	if (!s) {
		G__DEBUG(0);
		return 0;
	}
	memcpy(s, s_, g__strlen(s_));
	s[g__strlen(s_)] = 0;
	return s;
}

const char *
g__error(int e)
{
	switch (e) {
	case 0 /*--------*/ : return "^";
	case G__ERR_MEMORY  : return "ERR_MEMORY";
	case G__ERR_SYSTEM  : return "ERR_SYSTEM";
	case G__ERR_ARGUMENT: return "ERR_ARGUMENT";
	case G__ERR_SOFTWARE: return "ERR_SOFTWARE";
	case G__ERR_SYNTAX  : return "ERR_SYNTAX";
	case G__ERR_FILE    : return "ERR_FILE";
	case G__ERR_JITC    : return "ERR_JITC";
	}
	return "G__ERR_UNKNOWN";
}

void
g__unlink(const char *pathname)
{
	if (g__strlen(pathname)) {
		unlink(pathname);
	}
}
