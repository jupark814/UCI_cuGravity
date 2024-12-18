/**
 * g.h
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

#ifndef _G_H_
#define _G_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
#if(0)
}
#endif /* __cplusplus */

typedef struct g *g_t;

int g_version(void);

void g_debug(int enabled);

g_t g_open(const char *optimizer,
	   const char *precision,
	   const char *costfnc,
	   const char *batch,
	   const char *input,
	   const char *output,
	   /* hidden */ ...);

void g_close(g_t g);

size_t g_memory_size(g_t g);

size_t g_memory_hard(g_t g);

void *g_activate(g_t g, const void *x);

int g_train(g_t g, const void *x, const void *y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _G_H_ */
