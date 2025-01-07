/**
 * g_ir.h
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

#ifndef _G_IR_H_
#define _G_IR_H_

#include "g_common.h"

#define G__IR_OPTIMIZER_NONE 0
#define G__IR_OPTIMIZER_SGD  1

#define G__IR_PRECISION_NONE   0
#define G__IR_PRECISION_FLOAT  1
#define G__IR_PRECISION_DOUBLE 2
#define G__IR_PRECISION_FIXED  3

#define G__IR_COSTFNC_NONE          0
#define G__IR_COSTFNC_QUADRATIC     1
#define G__IR_COSTFNC_EXPONENTIAL   2
#define G__IR_COSTFNC_CROSS_ENTROPY 3

#define G__IR_ACTIVATION_NONE    0
#define G__IR_ACTIVATION_RELU    1
#define G__IR_ACTIVATION_LINEAR  2
#define G__IR_ACTIVATION_SOFTMAX 3
#define G__IR_ACTIVATION_SIGMOID 4

struct g__ir {
	int batch;
	int layers;
	int costfnc;
	int cuda;
	const char *module;
	const char *prefix;
	struct {
		int whole;
		int fraction;
		int precision;
	} precision;
	struct {
		int optimizer;
		double learning_rate;
	} optimizer;
	struct {
		int size;
		int activation;
	} *nodes;
};

const struct g__ir *g__ir_parse(const char *pathname);

void g__ir_destroy(void);

/*-----------------------------------------------------------------------------
 * Lexer/Parser Backend
 *---------------------------------------------------------------------------*/

extern int yylineno;
extern int yyerroron;
extern int yylex(void);
extern int yyparse(void);
extern int yylex_destroy(void);
extern void yyrestart(FILE *file);
extern void yyerror(const char *format, ...);

int g__ir_top(void);
int g__ir_precision(long whole, long fraction, long precision);
int g__ir_module(const char *s);
int g__ir_prefix(const char *s);
int g__ir_optimizer(long optimizer, double eta);
int g__ir_costfnc(long costfnc);
int g__ir_batch(long batch);
int g__ir_input(long size);
int g__ir_output(long size, long activation);
int g__ir_hidden(long size, long activation);
int g__ir_cuda(long cuda);
void *g__ir_malloc(size_t n);
char *g__ir_strdup(const char *s_);

#endif /* _G_IR_H_ */
