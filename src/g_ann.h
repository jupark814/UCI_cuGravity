/**
 * g_ann.h
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

#ifndef _G_ANN_H_
#define _G_ANN_H_

#include "g_ir.h"

#define G__ANN_OPTIMIZER_NONE G__IR_OPTIMIZER_NONE
#define G__ANN_OPTIMIZER_SGD  G__IR_OPTIMIZER_SGD

#define G__ANN_PRECISION_NONE   G__IR_PRECISION_NONE
#define G__ANN_PRECISION_FLOAT  G__IR_PRECISION_FLOAT
#define G__ANN_PRECISION_DOUBLE G__IR_PRECISION_DOUBLE
#define G__ANN_PRECISION_FIXED  G__IR_PRECISION_FIXED

#define G__ANN_PROGRAM_INITIALIZE 0
#define G__ANN_PROGRAM_ACTIVATE   1
#define G__ANN_PROGRAM_BACKPROP   2
#define G__ANN_PROGRAM_TRAIN      3
#define G__ANN_PROGRAM_END        4

#define G__ANN_PROGRAM_INST_RET         1
#define G__ANN_PROGRAM_INST_RETARG      2
#define G__ANN_PROGRAM_INST_BATCHLOOP   3
#define G__ANN_PROGRAM_INST_RANDOM     11
#define G__ANN_PROGRAM_INST_CLEAR      12
#define G__ANN_PROGRAM_INST_COPYX      13
#define G__ANN_PROGRAM_INST_MAC1       14
#define G__ANN_PROGRAM_INST_MAC2       15
#define G__ANN_PROGRAM_INST_MAC3       16
#define G__ANN_PROGRAM_INST_MAC4       17
#define G__ANN_PROGRAM_INST_ADD        18
#define G__ANN_PROGRAM_INST_SUBY       19
#define G__ANN_PROGRAM_INST_RELU      101
#define G__ANN_PROGRAM_INST_LINEAR    102
#define G__ANN_PROGRAM_INST_SOFTMAX   103
#define G__ANN_PROGRAM_INST_SIGMOID   104
#define G__ANN_PROGRAM_INST_RELUD    1001
#define G__ANN_PROGRAM_INST_LINEARD  1002
#define G__ANN_PROGRAM_INST_SOFTMAXD 1003
#define G__ANN_PROGRAM_INST_SIGMOIDD 1004

struct g__ann {
	int layers;
	const char *module;
	const char *prefix;
	struct g__ann_precision {
		int whole;
		int fraction;
		int precision;
		uint64_t size; /* bytes */
		uint64_t hard; /* bytes */
		uint64_t *w;   /* byte address */
		uint64_t *b;   /* byte address */
		uint64_t *a_;  /* byte address */
		uint64_t *d_;  /* byte address */
		uint64_t *w_;  /* byte address */
		uint64_t *b_;  /* byte address */
	} precision;
	struct g__ann_program {
		int size;
		struct g__ann_program_inst {
			int opc;
			int whole;
			int fraction;
			int precision;
			union {
				uint64_t i;
				double r;
			} arg[5];
		} *inst;
	} program[G__ANN_PROGRAM_END];
};

struct g__ann *g__ann_open(const struct g__ir *ir);

void g__ann_close(struct g__ann *an);

#endif /* _G__ANN_H_ */
