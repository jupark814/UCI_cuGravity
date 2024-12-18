/**
 * g_ann.c
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

#include "g_ann.h"

#define MAX_PROGRAM_CAPACITY 1000

struct g__ann_program_inst *
newinst(struct g__ann_program *program)
{
	if (MAX_PROGRAM_CAPACITY <= program->size) {
		G__DEBUG(G__ERR_SOFTWARE);
		assert( 0 );
		exit(-1);
		return 0;
	}
	return &program->inst[program->size++];
}

static size_t
unit(const struct g__ann_precision *precision)
{
	switch (precision->precision) {
	case G__IR_PRECISION_FLOAT : return 4;
	case G__IR_PRECISION_DOUBLE: return 8;
	case G__IR_PRECISION_FIXED : break;/* FIX : not implemented */
	default /*--------------*/ : break;
	}
	G__DEBUG(G__ERR_SOFTWARE);
	assert( 0 );
	exit(-1);
	return 0;
}

static int
emit_precision(struct g__ann *ann, const struct g__ir *ir)
{
	struct g__ann_precision *precision;
	uint64_t n, m;
	int l;

	precision = &ann->precision;
	precision->whole = ir->precision.whole;
	precision->fraction = ir->precision.fraction;
	precision->precision = ir->precision.precision;
	for (l=1; l<ir->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		precision->w[l] = precision->size;
		precision->size += unit(precision) * n * m;
		precision->b[l] = precision->size;
		precision->size += unit(precision) * n * 1;
	}
	precision->hard = precision->size;
	for (l=1; l<ir->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		precision->w_[l] = precision->size;
		precision->size += unit(precision) * n * m;
		precision->b_[l] = precision->size;
		precision->size += unit(precision) * n * 1;
	}
	for (l=0; l<ir->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		precision->a_[l] = precision->size;
		precision->size += unit(precision) * n * 1;
		if (l) {
			precision->d_[l] = precision->size;
			precision->size += unit(precision) * n * 1;
		}
	}
	return 0;
}

static int
emit_program_initialize(struct g__ann *ann,
			const struct g__ir *ir,
			int program_)
{
	struct g__ann_precision *precision;
	struct g__ann_program *program;
	struct g__ann_program_inst *inst;
	uint64_t n, m;
	int l;

	/* setup */

	precision = &ann->precision;
	program = &ann->program[program_];

	/* return */

	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_RET;
	inst->whole = precision->whole;
	inst->fraction = precision->fraction;
	inst->precision = precision->precision;

	/*
	 * w[*]
	 *   w[*] = random [-6.0 / (n + m) -- +6.0 / (n + m)]
	 *   b[*] = 0
	 */

	for (l=1; l<ann->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_RANDOM;
		inst->arg[0].i = precision->w[l];
		inst->arg[1].r = (-6.0 / (n + m)) * 1.0;
		inst->arg[2].r = (+6.0 / (n + m)) * 2.0;
		inst->arg[3].i = n * m;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_CLEAR;
		inst->arg[0].i = precision->b[l];
		inst->arg[1].i = n * 1;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
	}
	return 0;
}

static int
emit_program_activate(struct g__ann *ann, const struct g__ir *ir, int program_)
{
	struct g__ann_precision *precision;
	struct g__ann_program *program;
	struct g__ann_program_inst *inst;
	uint64_t n, m;
	int l;

	/* setup */

	precision = &ann->precision;
	program = &ann->program[program_];

	/*
	 * return:
	 *   y := a_[L]
	 */

	l = ann->layers;
	/*--*/
	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_RETARG;
	inst->arg[0].i = precision->a_[l - 1];
	inst->whole = precision->whole;
	inst->fraction = precision->fraction;
	inst->precision = precision->precision;

	/*
	 * a_[*]:
	 *    a_[0] := x
	 */

	n = (uint64_t)ir->nodes[0].size;
	/*--*/
	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_COPYX;
	inst->arg[0].i = precision->a_[0];
	inst->arg[1].i = n;
	inst->whole = precision->whole;
	inst->fraction = precision->fraction;
	inst->precision = precision->precision;

	/*
	 * a_[*]:
	 *    a_[l] := activation( w[l] * a_[l - 1] + b[l] )
	 *
	 * activation:
	 *    RELU
	 *    LINEAR
	 *    SOFTMAX
	 *    SIGMOID
	 */

	for (l=1; l<ann->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_MAC1;
		inst->arg[0].i = precision->a_[l];
		inst->arg[1].i = precision->w[l];
		inst->arg[2].i = precision->a_[l - 1];
		inst->arg[3].i = n;
		inst->arg[4].i = m;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_ADD;
		inst->arg[0].i = precision->a_[l];
		inst->arg[1].i = precision->b[l];
		inst->arg[2].i = n;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
		/*--*/
		inst = newinst(program);
		inst->opc = 100 + ir->nodes[l].activation;
		inst->arg[0].i = precision->a_[l];
		inst->arg[1].i = n;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
	}
	return 0;
}

static int
emit_program_backprop(struct g__ann *ann, const struct g__ir *ir, int program_)
{
	struct g__ann_precision *precision;
	struct g__ann_program *program;
	struct g__ann_program_inst *inst;
	uint64_t n, m;
	int l;

	/* setup */

	precision = &ann->precision;
	program = &ann->program[program_];

	/* return */

	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_RET;
	inst->whole = precision->whole;
	inst->fraction = precision->fraction;
	inst->precision = precision->precision;

	/*
	 * d_[*]:
	 *    d_[L] := a_[L] − y
	 */

	l = ann->layers - 1;
	n = (uint64_t)ir->nodes[l].size;
	if ((G__IR_COSTFNC_CROSS_ENTROPY != ir->costfnc) ||
	    (G__IR_ACTIVATION_SOFTMAX != ir->nodes[l].activation)) {

		/*
		 * Currently only cross_entropy cost function and
		 * softmax output activation is supported.
		 */

		G__DEBUG(G__ERR_SOFTWARE);
		return -1;
	}
	/*--*/
	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_SUBY;
	inst->arg[0].i = precision->d_[l];
	inst->arg[1].i = precision->a_[l];
	inst->arg[2].i = n;
	inst->whole = precision->whole;
	inst->fraction = precision->fraction;
	inst->precision = precision->precision;

	/*
	 * d_[*]:
	 *    d_[l] := (w[l+1]' * d_[l+1]) ⊙ σ′(a_[l])
	 *    d_[1] := (w[l+1]' * d_[l+1])
	 */

	while (1 < l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		if (G__IR_ACTIVATION_SOFTMAX == ir->nodes[l - 1].activation) {

			/*
			 * Currently not supporting softmax activation
			 * for hidden layers.
			 */

			G__DEBUG(G__ERR_SOFTWARE);
			return -1;
		}
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_MAC2;
		inst->arg[0].i = precision->d_[l - 1];
		inst->arg[1].i = precision->w[l];
		inst->arg[2].i = precision->d_[l];
		inst->arg[3].i = n;
		inst->arg[4].i = m;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
		/*--*/
		inst = newinst(program);
		inst->opc = 1000 + ir->nodes[l - 1].activation;
		inst->arg[0].i = precision->d_[l - 1];
		inst->arg[1].i = precision->a_[l - 1];
		inst->arg[2].i = m;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
		--l;
	}

	/*
	 * b_[*]:
	 *    b_[l] := b_[l] + d_[l]
	 *
	 * w_[*]:
	 *    w_[l] := w_[l] + d_[l] * a_[l - 1]
	 */

	for (l=1; l<ann->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_ADD;
		inst->arg[0].i = precision->b_[l];
		inst->arg[1].i = precision->d_[l];
		inst->arg[2].i = n;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_MAC3;
		inst->arg[0].i = precision->w_[l];
		inst->arg[1].i = precision->d_[l];
		inst->arg[2].i = precision->a_[l - 1];
		inst->arg[3].i = n;
		inst->arg[4].i = m;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
	}
	return 0;
}

static int
emit_program_train(struct g__ann *ann, const struct g__ir *ir, int program_)
{
	struct g__ann_precision *precision;
	struct g__ann_program *program;
	struct g__ann_program_inst *inst;
	uint64_t n, m, size;
	int l;

	/* setup */

	precision = &ann->precision;
	program = &ann->program[program_];

	/* return */

	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_RET;
	inst->whole = precision->whole;
	inst->fraction = precision->fraction;
	inst->precision = precision->precision;

	/*
	 * adjustments:
	 *   w_[*] := 0
	 *   b_[*] := 0
	 */

	size = 0;
	for (l=1; l<ann->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		size += n * m;
		size += n * 1;
	}
	/*--*/
	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_CLEAR;
	inst->arg[0].i = precision->w_[1];
	inst->arg[1].i = size;
	inst->whole = precision->whole;
	inst->fraction = precision->fraction;
	inst->precision = precision->precision;

	/*
	 * for each (x -> y) pair:
	 *   activate()
	 *   backprop()
	 */

	l = ann->layers;
	n = (uint64_t)ir->nodes[0].size;
	m = (uint64_t)ir->nodes[l - 1].size;
	/*--*/
	inst = newinst(program);
	inst->opc = G__ANN_PROGRAM_INST_BATCHLOOP;
	inst->arg[0].i = ir->batch;
	inst->arg[1].i = n;
	inst->arg[2].i = m;

	/*
	 * w[*]:
	 *    w_[l] := (η / k) * w_[l]
	 *    w[l] := w[l] - w_[l]
	 *
	 * b[*]:
	 *    b_[l] := (η / k) * b_[l]
	 *    b[l] := b[l] - b_[l]
	 */

	for (l=1; l<ann->layers; ++l) {
		n = (uint64_t)ir->nodes[l].size;
		m = (uint64_t)ir->nodes[l - 1].size;
		if (G__IR_OPTIMIZER_SGD != ir->optimizer.optimizer) {

			/*
			 * Currently only supporting SGD optimizer.
			 */

			G__DEBUG(G__ERR_SOFTWARE);
			return -1;
		}
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_MAC4;
		inst->arg[0].i = precision->w[l];
		inst->arg[1].i = precision->w_[l];
		inst->arg[2].r = -((double)ir->optimizer.learning_rate /
				   (double)ir->batch);
		inst->arg[3].i = n * m;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
		/*--*/
		inst = newinst(program);
		inst->opc = G__ANN_PROGRAM_INST_MAC4;
		inst->arg[0].i = precision->b[l];
		inst->arg[1].i = precision->b_[l];
		inst->arg[2].r = -((double)ir->optimizer.learning_rate /
				   (double)ir->batch);
		inst->arg[3].i = n * 1;
		inst->whole = precision->whole;
		inst->fraction = precision->fraction;
		inst->precision = precision->precision;
	}
	return 0;
}

static int
emit_program(struct g__ann *ann, const struct g__ir *ir)
{
	if (emit_program_initialize(ann,
				    ir,
				    G__ANN_PROGRAM_INITIALIZE) ||
	    emit_program_activate(ann,
				  ir,
				  G__ANN_PROGRAM_ACTIVATE) ||
	    emit_program_backprop(ann,
				  ir,
				  G__ANN_PROGRAM_BACKPROP) ||
	    emit_program_train(ann,
			       ir,
			       G__ANN_PROGRAM_TRAIN)) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

struct g__ann *
g__ann_open(const struct g__ir *ir)
{
	struct g__ann_precision *precision;
	struct g__ann *ann;
	size_t n;
	int i;

	assert( ir );

	/* initialize */

	ann = g__malloc(sizeof (struct g__ann));
	if (!ann) {
		G__DEBUG(0);
		return 0;
	}
	memset(ann, 0, sizeof (struct g__ann));
	ann->layers = ir->layers;
	ann->module = g__strdup(ir->module);
	ann->prefix = g__strdup(ir->prefix);
	if (!ann->module || !ann->prefix) {
		g__ann_close(ann);
		G__DEBUG(0);
		return 0;
	}

	/* memories */

	n = ir->layers * sizeof (uint64_t);
	precision = &ann->precision;
	precision->w = g__malloc(n);
	precision->b = g__malloc(n);
	precision->a_ = g__malloc(n);
	precision->d_ = g__malloc(n);
	precision->w_ = g__malloc(n);
	precision->b_ = g__malloc(n);
	if (!precision->w ||
	    !precision->b ||
	    !precision->a_ ||
	    !precision->d_ ||
	    !precision->w_ ||
	    !precision->b_) {
		g__ann_close(ann);
		G__DEBUG(0);
		return 0;
	}
	memset(precision->w, 0, n);
	memset(precision->b, 0, n);
	memset(precision->a_, 0, n);
	memset(precision->d_, 0, n);
	memset(precision->w_, 0, n);
	memset(precision->b_, 0, n);

	/* programs */

	for (i=0; i<G__ANN_PROGRAM_END; ++i) {
		n = MAX_PROGRAM_CAPACITY * sizeof (struct g__ann_program_inst);
		ann->program[i].inst = g__malloc(n);
		if (!ann->program[i].inst) {
			g__ann_close(ann);
			G__DEBUG(0);
			return 0;
		}
		memset(ann->program[i].inst, 0, n);
	}
	if (emit_precision(ann, ir) || emit_program(ann, ir)) {
		g__ann_close(ann);
		G__DEBUG(0);
		return 0;
	}
	return ann;
}

void
g__ann_close(struct g__ann *ann)
{
	struct g__ann_precision *precision;
	int i;

	if (ann) {
		precision = &ann->precision;
		G__FREE(precision->w);
		G__FREE(precision->b);
		G__FREE(precision->a_);
		G__FREE(precision->d_);
		G__FREE(precision->w_);
		G__FREE(precision->b_);
		for (i=0; i<G__ANN_PROGRAM_END; ++i) {
			G__FREE(ann->program[i].inst);
		}
		G__FREE(ann->module);
		G__FREE(ann->prefix);
		memset(ann, 0, sizeof (struct g__ann));
	}
	G__FREE(ann);
}
