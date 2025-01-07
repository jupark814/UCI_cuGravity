/**
 * g_ir.c
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

#include "g_ir.h"

#define MAX_CUDA	   1
#define MAX_BATCH   1000
#define MAX_SIZE 1000000

#define MARK_MODULE    0
#define MARK_PREFIX    1
#define MARK_OPTIMIZER 2
#define MARK_PRECISION 3
#define MARK_COSTFNC   4
#define MARK_BATCH     5
#define MARK_INPUT     6
#define MARK_OUTPUT    7
#define MARK_HIDDEN    8
#define MARK_CUDA	   9
#define MARK_END       10

#define NODE_TYPE_INPUT  0
#define NODE_TYPE_OUTPUT 1
#define NODE_TYPE_HIDDEN 2

static struct {
	void *_mem_;
	struct g__ir *ir;
	int mark[MARK_END];
	struct node {
		int type;
		int size;
		int activation;
		struct node *link;
	} *root;
} state;

static int
validc(const char *s)
{
	if (*s) {
		if (('_' != *s) && !isalpha(*s)) {
			return -1;
		}
		while (*s) {
			if (('_' != *s) && !isalnum(*s)) {
				return -1;
			}
			++s;
		}
	}
	return 0;
}

const struct g__ir *
g__ir_parse(const char *pathname)
{
	FILE *file;

	assert( !state._mem_ && g__strlen(pathname) );

	file = fopen(pathname, "r");
	if (!file) {
		yyerror("unable to open '%s' for reading", pathname);
		g__ir_destroy();
		G__DEBUG(G__ERR_FILE);
		return 0;
	}
	state.ir = g__ir_malloc(sizeof (struct g__ir));
	if (!state.ir) {
		yyerror("out of memory");
		fclose(file);
		g__ir_destroy();
		G__DEBUG(0);
		return 0;
	}
	yyrestart(file);
	if (yyparse()) {
		fclose(file);
		g__ir_destroy();
		G__DEBUG(0);
		return 0;
	}
	fclose(file);
	return state.ir;
}

void
g__ir_destroy(void)
{
	void **link;

	link = state._mem_;
	while (state._mem_) {
		link = state._mem_;
		state._mem_ = (*link);
		G__FREE(link);
	}
	yylex_destroy();
	memset(&state, 0, sizeof (state));
}

/*-----------------------------------------------------------------------------
 * Lexer/Parser Backend
 *---------------------------------------------------------------------------*/

int
g__ir_top(void)
{
	struct node *node;
	int i, n;
	int start, end, temp_size, temp_act;

	if (!state.mark[MARK_MODULE]) {
		yyerror("missing .module sepcification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (!state.mark[MARK_PREFIX]) {
		state.ir->prefix = "g";
	}
	if (!state.mark[MARK_OPTIMIZER]) {
		state.ir->optimizer.optimizer = G__IR_OPTIMIZER_SGD;
		state.ir->optimizer.learning_rate = 0.1;
	}
	if (!state.mark[MARK_PRECISION]) {
		state.ir->precision.whole = 0;
		state.ir->precision.fraction = 0;
		state.ir->precision.precision = G__IR_PRECISION_FLOAT;
	}
	if (!state.mark[MARK_COSTFNC]) {
		state.ir->costfnc = G__IR_COSTFNC_CROSS_ENTROPY;
	}
	if (!state.mark[MARK_BATCH]) {
		state.ir->batch = 1;
	}
	if (!state.mark[MARK_INPUT]) {
		yyerror("missing .input specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (!state.mark[MARK_OUTPUT]) {
		yyerror("missing .output specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (!state.mark[MARK_HIDDEN]) {
		yyerror("missing .hidden specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (!state.mark[MARK_CUDA]) {
		state.ir->cuda = 0;
	}
	n = 2 + state.mark[MARK_HIDDEN];
	state.ir->layers = n;
	state.ir->nodes = g__ir_malloc(n * sizeof (state.ir->nodes[0]));
	if (!state.ir->nodes) {
		yyerror("out of memory");
		G__DEBUG(0);
		return -1;
	}
	i = 0;
	node = state.root;
	while (node) {
		if (NODE_TYPE_INPUT == node->type) {
			state.ir->nodes[i].size = node->size;
			state.ir->nodes[i].activation = node->activation;
			++i;
		}
		node = node->link;
	}
	node = state.root;
	while (node) {
		if (NODE_TYPE_HIDDEN == node->type) {
			state.ir->nodes[i].size = node->size;
			state.ir->nodes[i].activation = node->activation;
			++i;
		}
		node = node->link;
	}

	start = 1;
	end = state.mark[MARK_HIDDEN];
	while (start < end) {
		temp_size = state.ir->nodes[start].size;
		temp_act = state.ir->nodes[start].activation;
		state.ir->nodes[start] = state.ir->nodes[end];
		state.ir->nodes[end].size = temp_size;
		state.ir->nodes[end].activation = temp_act;
		++start;
		--end;
	}

	node = state.root;
	while (node) {
		if (NODE_TYPE_OUTPUT == node->type) {
			state.ir->nodes[i].size = node->size;
			state.ir->nodes[i].activation = node->activation;
			++i;
		}
		node = node->link;
	}
	return 0;
}

int
g__ir_module(const char *s)
{
	if (state.mark[MARK_MODULE]) {
		yyerror("duplicate .module specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (!g__strlen(s) || validc(s)) {
		yyerror("invalid .module specification '%s' ", s);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	state.ir->module = s;
	state.mark[MARK_MODULE] += 1;
	return 0;
}

int
g__ir_prefix(const char *s)
{
	if (state.mark[MARK_PREFIX]) {
		yyerror("duplicate .prefix specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (validc(s)) {
		yyerror("invalid .prefix specification '%s' ", s);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	state.ir->prefix = s;
	state.mark[MARK_PREFIX] += 1;
	return 0;
}

int
g__ir_optimizer(long optimizer, double learning_rate)
{
	if (state.mark[MARK_OPTIMIZER]) {
		yyerror("duplicate .optimizer specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if ((0.0 >= learning_rate) || (1.0 < learning_rate)) {
		yyerror("invalid .optimizer specification '%f'",
			learning_rate);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	state.ir->optimizer.optimizer = (int)optimizer;
	state.ir->optimizer.learning_rate = learning_rate;
	state.mark[MARK_OPTIMIZER] += 1;
	return 0;
}

int
g__ir_precision(long whole, long fraction, long precision)
{
	if (state.mark[MARK_PRECISION]) {
		yyerror("duplicate .precision specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if ((G__IR_PRECISION_FIXED == precision) &&
	    ((1 > whole) || (0 > fraction) ||
	     (64 < (whole + fraction)))) {
		yyerror("invalid .precision 'FIXED [%ld, %ld]' ",
			whole,
			fraction);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	state.ir->precision.whole = (int)whole;
	state.ir->precision.fraction = (int)fraction;
	state.ir->precision.precision = (int)precision;
	state.mark[MARK_PRECISION] += 1;
	return 0;
}

int
g__ir_costfnc(long costfnc)
{
	if (state.mark[MARK_COSTFNC]) {
		yyerror("duplicate .costfnc specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	state.ir->costfnc = (int)costfnc;
	state.mark[MARK_COSTFNC] += 1;
	return 0;
}

int
g__ir_batch(long batch)
{
	if (state.mark[MARK_BATCH]) {
		yyerror("duplicate .batch specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (MAX_BATCH < batch) {
		yyerror("invalid .batch specification '%ld'", batch);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	state.ir->batch = (int)batch;
	state.mark[MARK_BATCH] += 1;
	return 0;
}

int
g__ir_input(long size)
{
	struct node *node;

	if (state.mark[MARK_INPUT]) {
		yyerror("duplicate .input specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (MAX_SIZE < size) {
		yyerror("invalid .input specification '%ld'", size);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	node = g__ir_malloc(sizeof (struct node));
	if (!node) {
		yyerror("out of memory");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	node->type = NODE_TYPE_INPUT;
	node->size = (int)size;
	node->link = state.root;
	state.root = node;
	state.mark[MARK_INPUT] += 1;
	return 0;
}

int
g__ir_output(long size, long activation)
{
	struct node *node;

	if (state.mark[MARK_OUTPUT]) {
		yyerror("duplicate .output specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (MAX_SIZE < size) {
		yyerror("invalid .output specification '%ld'", size);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	node = g__ir_malloc(sizeof (struct node));
	if (!node) {
		yyerror("out of memory");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	node->type = NODE_TYPE_OUTPUT;
	node->size = (int)size;
	node->activation = (int)activation;
	node->link = state.root;
	state.root = node;
	state.mark[MARK_OUTPUT] += 1;
	return 0;
}

int
g__ir_hidden(long size, long activation)
{
	struct node *node;

	if (MAX_SIZE < size) {
		yyerror("invalid .hidden specification '%ld'", size);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	node = g__ir_malloc(sizeof (struct node));
	if (!node) {
		yyerror("out of memory");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	node->type = NODE_TYPE_HIDDEN;
	node->size = (int)size;
	node->activation = (int)activation;
	node->link = state.root;
	state.root = node;
	state.mark[MARK_HIDDEN] += 1;
	return 0;
}

int
g__ir_cuda(long cuda)
{
	if (state.mark[MARK_CUDA]) {
		yyerror("duplicate .CUDA specification");
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	if (MAX_CUDA < cuda) {
		yyerror("invalid .CUDA specification '%ld'", cuda);
		G__DEBUG(G__ERR_SYNTAX);
		return -1;
	}
	state.ir->cuda = (int)cuda;
	state.mark[MARK_CUDA] += 1;
	return 0;
}

void *
g__ir_malloc(size_t n)
{
	void **link;

	assert( n );

	n += sizeof (link);
	if (!(link = g__malloc(n))) {
		G__DEBUG(0);
		return 0;
	}
	memset(link, 0, n);
	(*link) = state._mem_;
	state._mem_ = link;
	return (link + 1);
}

char *
g__ir_strdup(const char *s_)
{
	char *s;

	assert( s_ );

	if (!(s = g__ir_malloc(g__strlen(s_) + 1))) {
		G__DEBUG(0);
		return 0;
	}
	strcpy(s, s_);
	return s;
}
