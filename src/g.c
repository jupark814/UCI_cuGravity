/**
 * g.c
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

#include "g_emitc.h"
#include "g_vcm.h"
#include "g.h"

#define SIG 1298343576
#define MAX_LAYERS  10

typedef int    (*version_fnc_t)     (void);
typedef size_t (*memory_size_fnc_t) (void);
typedef size_t (*memory_hard_fnc_t) (void);
typedef void   (*initialize_fnc_t)  (void *);
typedef void  *(*activate_fnc_t)    (void *, const void *);
typedef void   (*train_fnc_t)       (void *, const void *, const void *);

struct g {
	void *memory;
	unsigned sig;
	g__vcm_t vcm;
	version_fnc_t version;
	memory_size_fnc_t memory_size;
	memory_hard_fnc_t memory_hard;
	initialize_fnc_t initialize;
	activate_fnc_t activate;
	train_fnc_t train;
};

static int
populate(const char *pathname,
	 const char *module,
	 const char *prefix,
	 const char *optimizer,
	 const char *precision,
	 const char *costfnc,
	 const char *batch,
	 const char *input,
	 const char *output,
	 const char *layers[])
{
	FILE *file;
	int i;

	file = fopen(pathname, "w");
	if (!file) {
		G__DEBUG(G__ERR_FILE);
		return -1;
	}
	if (0 > fprintf(file,
			"%s ;\n"
			"%s ;\n"
			"%s ;\n"
			"%s ;\n"
			"%s ;\n"
			"%s ;\n"
			"%s ;\n"
			"%s ;\n",
			module,
			prefix,
			optimizer,
			precision,
			costfnc,
			batch,
			input,
			output)) {
		fclose(file);
		G__DEBUG(G__ERR_FILE);
		return -1;
	}
	for (i=0; layers[i]; ++i) {
		if (0 > fprintf(file, "%s ;\n", layers[i])) {
			fclose(file);
			G__DEBUG(G__ERR_FILE);
			return -1;
		}
	}
	fclose(file);
	return 0;
}

int
g_version(void)
{
	return G__VERSION;
}

void
g_debug(int enabled)
{
	yyerroron = 0;
	g__debug_enabled = 0;
	if (enabled) {
		yyerroron = 1;
		g__debug_enabled = 1;
	}
}

g_t
g_open(const char *optimizer,
       const char *precision,
       const char *costfnc,
       const char *batch,
       const char *input,
       const char *output,
       /* hidden */ ...)
{
	const char *tmp, module[256], *layers[1+MAX_LAYERS];
	const struct g__ir *ir;
	struct g__ann *ann;
	struct g *g;
	va_list va;
	int tag, i;
	size_t n;
	char *s;

	/* check arguments */

	i = 0;
	tag = rand();
	memset(layers, 0, sizeof (layers));
	if (!g__strlen(optimizer) ||
	    !g__strlen(precision) ||
	    !g__strlen(costfnc) ||
	    !g__strlen(batch) ||
	    !g__strlen(input) ||
	    !g__strlen(output)) {
		G__DEBUG(G__ERR_ARGUMENT);
		return 0;
	}
	va_start(va, output);
	do {
		layers[i] = va_arg(va, const char *);
		if (MAX_LAYERS <= i) {
			va_end(va);
			G__DEBUG(G__ERR_ARGUMENT);
			return 0;
		}
	}
	while (layers[i++]);
	va_end(va);
	if (!layers[0]) {
		G__DEBUG(G__ERR_ARGUMENT);
		return 0;
	}

	/* initialize */

	g = g__malloc(sizeof (struct g));
	if (!g) {
		G__DEBUG(0);
		return 0;
	}
	memset(g, 0, sizeof (struct g));
	g->sig = SIG;

	/* populate */

	tmp = getenv("TMPDIR");
	tmp = tmp ? tmp : getenv("TMP");
	tmp = tmp ? tmp : getenv("TEMP");
	tmp = tmp ? tmp : ".";
	n = g__strlen(tmp) + 32;
	s = g__malloc(n);
	if (!s) {
		g_close(g);
		G__DEBUG(0);
		return 0;
	}
	g__sprintf(s, n, "%s/_%x_.g", tmp, tag);
	g__sprintf((char *)module, sizeof (module), ".module \"_%x_\"", tag);
	if (populate(s,
		     module,
		     ".prefix \"\"",
		     optimizer,
		     precision,
		     costfnc,
		     batch,
		     input,
		     output,
		     layers)) {
		g_close(g);
		G__FREE(s);
		G__DEBUG(0);
		return 0;
	}

	/* g compile */

	ir = g__ir_parse(s);
	g__unlink(s);
	if (!ir) {
		g_close(g);
		G__FREE(s);
		G__DEBUG(0);
		return 0;
	}
	ann = g__ann_open(ir);
	g__ir_destroy();
	if (!ann || g__emitc(ann, tmp)) {
		g__ann_close(ann);
		g_close(g);
		G__FREE(s);
		G__DEBUG(0);
		return 0;
	}
	g__ann_close(ann);

	/* c compile */

	g__sprintf(s, n, "%s/_%x_.c", tmp, tag);
	g->vcm = g__vcm_open(s);
	g__unlink(s);
	g__sprintf(s, n, "%s/_%x_.h", tmp, tag);
	g__unlink(s);
	G__FREE(s);
	if (!g->vcm) {
		g_close(g);
		G__DEBUG(0);
		return 0;
	}

	/* jit connect */

	g->version = (version_fnc_t)(long)
		g__vcm_lookup(g->vcm, "_version");
	g->memory_size = (memory_size_fnc_t)(long)
		g__vcm_lookup(g->vcm, "_memory_size");
	g->memory_hard = (memory_hard_fnc_t)(long)
		g__vcm_lookup(g->vcm, "_memory_hard");
	g->initialize = (initialize_fnc_t)(long)
		g__vcm_lookup(g->vcm, "_initialize");
	g->activate = (activate_fnc_t)(long)
		g__vcm_lookup(g->vcm, "_activate");
	g->train = (train_fnc_t)(long)
		g__vcm_lookup(g->vcm, "_train");
	assert( g->version &&
		g->memory_size &&
		g->memory_hard &&
		g->initialize &&
		g->activate &&
		g->train );
	assert( G__VERSION == g->version() );

	/* allocate ANN memory */

	g->memory = g__malloc(g->memory_size());
	if (!g->memory) {
		g_close(g);
		G__DEBUG(0);
		return 0;
	}
	memset(g->memory, 0, g->memory_size());
	g->initialize(g->memory);
	return g;
}

void
g_close(g_t g)
{
	if (g && (SIG == g->sig)) {
		g__vcm_close(g->vcm);
		G__FREE(g->memory);
		memset(g, 0, sizeof (struct g));
		G__FREE(g);
	}
}

size_t
g_memory_size(g_t g)
{
	if (!g || (SIG != g->sig)) {
		G__DEBUG(G__ERR_ARGUMENT);
		return 0;
	}
	return g->memory_size();
}

size_t
g_memory_hard(g_t g)
{
	if (!g || (SIG != g->sig)) {
		G__DEBUG(G__ERR_ARGUMENT);
		return 0;
	}
	return g->memory_hard();
}

void *
g_activate(g_t g, const void *x)
{
	if (!g || (SIG != g->sig) || !x) {
		G__DEBUG(G__ERR_ARGUMENT);
		return 0;
	}
	return g->activate(g->memory, x);
}

int
g_train(g_t g, const void *x, const void *y)
{
	if (!g || (SIG != g->sig) | !x || !y) {
		G__DEBUG(G__ERR_ARGUMENT);
		return -1;
	}
	g->train(g->memory, x, y);
	return 0;
}
