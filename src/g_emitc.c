/**
 * g_emitc.c
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

#define UL(x) ((unsigned long)(x))

#define P print

static const char *
capitalize(const char *s_)
{
	size_t i;
	char *s;

	s = g__malloc(g__strlen(s_) + 1);
	if (!s) {
		G__DEBUG(0);
		return 0;
	}
	for (i=0; i<g__strlen(s_); ++i) {
		s[i] = toupper(s_[i]);
	}
	s[i] = 0;
	return s;
}

static int
print(FILE *file, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if (0 > vfprintf(file, format, ap)) {
		va_end(ap);
		G__DEBUG(G__ERR_FILE);
		return -1;
	}
	va_end(ap);
	return 0;
}

static const char *
precision(const struct g__ann_program_inst *inst)
{
	switch (inst->precision) {
	case G__ANN_PRECISION_FLOAT : return "float";
	case G__ANN_PRECISION_DOUBLE: return "double";
	case G__ANN_PRECISION_FIXED : break; /* FIX : not implemented */
	default /*---------------*/ : break;
	}
	G__DEBUG(G__ERR_SOFTWARE);
	assert( 0 );
	exit(-1);
	return 0;
}

static const char *
type(uint64_t x)
{
	if (0xff >= x) {
		return "uint32_t";
	}
	else if (0xffff >= x) {
		return "uint32_t";
	}
	else if (0xffffffff >= x) {
		return "uint32_t";
	}
	return "uint64_t";
}

static int
inst_ret(const struct g__ann_program_inst *inst, FILE *file)
{
	G__UNUSED(inst);
	if (P(file,
	      "  { /* RET */\n"
	      "    return;\n"
	      "  }\n")) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_retarg(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* RETARG */\n"
	      "    return (%s *)( m_ + %lu );\n"
	      "  }\n",
	      precision(inst),
	      UL(inst->arg[0].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_batchloop(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* BATCHLOOP */\n"
	      "    %s i;\n"
	      "    for (i=0; i<%lu; ++i) {\n"
	      "        _activate_(m_, x_ + i * %lu);\n"
	      "        _backprop_(m_, y_ + i * %lu);\n"
	      "    }\n"
	      "  }\n\n",
	      type(inst->arg[0].i * G__MAX(inst->arg[1].i, inst->arg[2].i)),
	      UL(inst->arg[0].i),
	      UL(inst->arg[1].i),
	      UL(inst->arg[2].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_random(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* RANDOM */\n"
	      "    %s r, *z = (%s *)( m_ + %lu );\n"
	      "    %s i;\n"
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      r = (%s)rand() / RAND_MAX;\n"
	      "      z[i] = %f + r * %f;\n"
	      "    }\n"
	      "  }\n\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      type(inst->arg[3].i),
	      UL(inst->arg[3].i),
	      precision(inst),
	      inst->arg[1].r,
	      inst->arg[2].r)) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_clear(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* CLEAR */\n"
	      "    memset(m_ + %lu, 0, %lu * sizeof (%s));\n"
	      "  }\n\n",
	      UL(inst->arg[0].i),
	      UL(inst->arg[1].i),
	      precision(inst))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_copyx(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* COPYX */\n"
	      "    memcpy(m_ + %lu, x_, %lu * sizeof (%s));\n"
	      "  }\n\n",
	      UL(inst->arg[0].i),
	      UL(inst->arg[1].i),
	      precision(inst))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_mul1(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* MAC1 */\n"
	      "    %s *z = (%s *)( m_ + %lu );\n"
	      "    const %s *A = (const %s *)( m_ + %lu );\n"
	      "    const %s *B = (const %s *)( m_ + %lu );\n"
	      "    %s i, j;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[1].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[2].i),
	      type(G__MAX(inst->arg[3].i, inst->arg[4].i))) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      z[i] = 0.0;\n"
	      "      for (j=0; j<%lu; ++j) {\n"
	      "        z[i] += A[i * %lu + j] * B[j];\n"
	      "      }\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[3].i),
	      UL(inst->arg[4].i),
	      UL(inst->arg[4].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_mul2(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* MAC2 */\n"
	      "    %s *z = (%s *)( m_ + %lu );\n"
	      "    const %s *A = (const %s *)( m_ + %lu );\n"
	      "    const %s *B = (const %s *)( m_ + %lu );\n"
	      "    %s i, j;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[1].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[2].i),
	      type(G__MAX(inst->arg[3].i, inst->arg[4].i))) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      z[i] = 0.0;\n"
	      "      for (j=0; j<%lu; ++j) {\n"
	      "        z[i] += A[j * %lu + i] * B[j];\n"
	      "      }\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[4].i),
	      UL(inst->arg[3].i),
	      UL(inst->arg[4].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_mul3(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* MAC3 */\n"
	      "    %s *za = (%s *)( m_ + %lu );\n"
	      "    const %s *B = (const %s *)( m_ + %lu );\n"
	      "    const %s *C = (const %s *)( m_ + %lu );\n"
	      "    %s i, j;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[1].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[2].i),
	      type(G__MAX(inst->arg[3].i, inst->arg[4].i))) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      for (j=0; j<%lu; ++j) {\n"
	      "        za[i * %lu + j] += B[i] * C[j];\n"
	      "      }\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[3].i),
	      UL(inst->arg[4].i),
	      UL(inst->arg[4].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_mul4(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* MAC4 */\n"
	      "    %s *za = (%s *)( m_ + %lu );\n"
	      "    const %s *B = (const %s *)( m_ + %lu );\n"
	      "    %s i;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[1].i),
	      type(inst->arg[3].i)) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      za[i] += B[i] * %f;\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[3].i),
	      inst->arg[2].r)) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_add(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* ADD */\n"
	      "    %s *za = (%s *)( m_ + %lu );\n"
	      "    const %s *B = (const %s *)( m_ + %lu );\n"
	      "    %s i;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[1].i),
	      type(inst->arg[2].i)) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      za[i] += B[i];\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[2].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_suby(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* SUBY */\n"
	      "    %s *z = (%s *)( m_ + %lu );\n"
	      "    const %s *A = (const %s *)( m_ + %lu );\n"
	      "    %s i;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[1].i),
	      type(inst->arg[2].i)) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      z[i] = A[i] - y_[i];\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[2].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_relu(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* RELU */\n"
	      "    %s *za = (%s *)( m_ + %lu );\n"
	      "    %s i;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      type(inst->arg[1].i)) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      if (0.0 >= za[i]) {\n"
	      "        za[i] = 0.0;\n"
	      "      }\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[1].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_linear(const struct g__ann_program_inst *inst, FILE *file)
{
	G__UNUSED(inst);
	if (P(file,
	      "  { /* LINEAR */\n"
	      "    /* nothing to do */\n"
	      "  }\n\n")) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_softmax(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* SOFTMAX */\n"
	      "    %s *za = (%s *)( m_ + %lu );\n"
	      "    %s max=za[0], sum=0.0;\n"
	      "    %s i;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      type(inst->arg[1].i)) ||
	    P(file,
	      "    for (i=1; i<%lu; ++i) {\n"
	      "      if (max < za[i]) {\n"
	      "        max = za[i];\n"
	      "      }\n"
	      "    }\n"
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      za[i] -= max;\n"
	      "      sum += (%s)exp(za[i]);\n"
	      "    }\n"
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      za[i] = (%s)exp(za[i]) / sum;\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[1].i),
	      UL(inst->arg[1].i),
	      precision(inst),
	      UL(inst->arg[1].i),
	      precision(inst))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_sigmoid(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* SIGMOID */\n"
	      "    %s *za = (%s *)( m_ + %lu );\n"
	      "    %s zee;\n"
	      "    %s i;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      type(inst->arg[1].i)) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      if (0.0 <= za[i]) {\n"
	      "        zee = (%s)exp(-za[i]);\n"
	      "        za[i] = 1.0 / (1.0 + zee);\n"
	      "      }\n"
	      "      else {\n"
	      "        zee = (%s)exp(za[i]);\n"
	      "        za[i] = zee / (1.0 + zee);\n"
	      "      }\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[1].i),
	      precision(inst),
	      precision(inst))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_relud(const struct g__ann_program_inst *inst, FILE *file)
{
	if (P(file,
	      "  { /* RELUD */\n"
	      "    %s *za = (%s *)( m_ + %lu );\n"
	      "    const %s *B = (const %s *)( m_ + %lu );\n"
	      "    %s i;\n",
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[0].i),
	      precision(inst),
	      precision(inst),
	      UL(inst->arg[1].i),
	      type(inst->arg[2].i)) ||
	    P(file,
	      "    for (i=0; i<%lu; ++i) {\n"
	      "      if (0.0 >= B[i]) {\n"
	      "        za[i] = 0.0;\n"
	      "      }\n"
	      "    }\n"
	      "  }\n\n",
	      UL(inst->arg[2].i))) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
inst_lineard(const struct g__ann_program_inst *inst, FILE *file)
{
	G__UNUSED(inst);
	G__UNUSED(file);
	G__DEBUG(G__ERR_SOFTWARE);
	assert( 0 );
	exit(-1);
	return -1;
}

static int
inst_softmaxd(const struct g__ann_program_inst *inst, FILE *file)
{
	G__UNUSED(inst);
	G__UNUSED(file);
	G__DEBUG(G__ERR_SOFTWARE);
	assert( 0 );
	exit(-1);
	return -1;
}

static int
inst_sigmoidd(const struct g__ann_program_inst *inst, FILE *file)
{
	G__UNUSED(inst);
	G__UNUSED(file);
	G__DEBUG(G__ERR_SOFTWARE);
	assert( 0 );
	exit(-1);
	return -1;
}

static int
program(const struct g__ann_program *program, FILE *file)
{
	const struct g__ann_program_inst *inst;
	int i;

	for (i=1; i<program->size; ++i) {
		inst = &program->inst[i];
		if (G__ANN_PROGRAM_INST_BATCHLOOP == inst->opc) {
			if (inst_batchloop(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_RANDOM == inst->opc) {
			if (inst_random(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_CLEAR == inst->opc) {
			if (inst_clear(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_COPYX == inst->opc) {
			if (inst_copyx(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_MAC1 == inst->opc) {
			if (inst_mul1(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_MAC2 == inst->opc) {
			if (inst_mul2(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_MAC3 == inst->opc) {
			if (inst_mul3(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_MAC4 == inst->opc) {
			if (inst_mul4(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_ADD == inst->opc) {
			if (inst_add(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_SUBY == inst->opc) {
			if (inst_suby(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_RELU == inst->opc) {
			if (inst_relu(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_LINEAR == inst->opc) {
			if (inst_linear(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_SOFTMAX == inst->opc) {
			if (inst_softmax(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_SIGMOID == inst->opc) {
			if (inst_sigmoid(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_RELUD == inst->opc) {
			if (inst_relud(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_LINEARD == inst->opc) {
			if (inst_lineard(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_SOFTMAXD == inst->opc) {
			if (inst_softmaxd(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else if (G__ANN_PROGRAM_INST_SIGMOIDD == inst->opc) {
			if (inst_sigmoidd(inst, file)) {
				G__DEBUG(0);
				return -1;
			}
		}
		else {
			G__DEBUG(G__ERR_SOFTWARE);
			assert( 0 );
			exit(-1);
			return -1;
		}
	}
	inst = &program->inst[0];
	if (G__ANN_PROGRAM_INST_RET == inst->opc) {
		if (inst_ret(inst, file)) {
			G__DEBUG(0);
			return -1;
		}
	}
	else if (G__ANN_PROGRAM_INST_RETARG == inst->opc) {
		if (inst_retarg(inst, file)) {
			G__DEBUG(0);
			return -1;
		}
	}
	else {
		G__DEBUG(G__ERR_SOFTWARE);
		assert( 0 );
		exit(-1);
		return -1;
	}
	return 0;
}

static int
header(const struct g__ann *ann, FILE *file, int includes)
{
	time_t t;

	t = time(0);
	if (P(file,
	      "/*\n"
	      " * Auto Generated by The Gravity Compiler - %s"
	      " * Copyright (C) Tony Givargis, 2019-2020\n"
	      " */\n\n",
	      ctime(&t),
	      ann->module)) {
	}
	if (includes) {
		if (P(file,
		      "#include <stdlib.h>\n"
		      "#include <stdint.h>\n"
		      "#include <string.h>\n"
		      "#include <math.h>\n"
		      "#include \"%s.h\"\n\n",
		      ann->module)) {
			G__DEBUG(0);
			return -1;
		}
	}
	return 0;
}

static int
initialize(const struct g__ann *ann, FILE *file)
{
	const struct g__ann_program *prog;

	prog = &ann->program[G__ANN_PROGRAM_INITIALIZE];
	if (P(file, "static void _initialize_(char *m_) {\n") ||
	    program(prog, file) ||
	    P(file, "}\n\n")) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
activate(const struct g__ann *ann, FILE *file)
{
	const struct g__ann_program *prog;

	prog = &ann->program[G__ANN_PROGRAM_ACTIVATE];
	if (P(file,
	      "static %s *_activate_(char *m_, const %s *x_) {\n",
	      precision(&prog->inst[0]),
	      precision(&prog->inst[0])) ||
	    program(prog, file) ||
	    P(file, "}\n\n")) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
backprop(const struct g__ann *ann, FILE *file)
{
	const struct g__ann_program *prog;

	prog = &ann->program[G__ANN_PROGRAM_BACKPROP];
	if (P(file,
	      "static void _backprop_(char *m_, const %s *y_) {\n",
	      precision(&prog->inst[0])) ||
	    program(prog, file) ||
	    P(file, "}\n\n")) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
train(const struct g__ann *ann, FILE *file)
{
	const struct g__ann_program *prog;

	prog = &ann->program[G__ANN_PROGRAM_TRAIN];
	if (P(file,
	      "static void _train_(char *m_, const %s *x_, const %s *y_) {\n",
	      precision(&prog->inst[0]),
	      precision(&prog->inst[0])) ||
	    program(prog, file) ||
	    P(file, "}\n\n")) {
		G__DEBUG(0);
		return -1;
	}
	return 0;
}

static int
export(const struct g__ann *ann, FILE *file1, FILE *file2)
{
	const struct g__ann_program_inst *inst1, *inst2;
	const char *prefix;

	inst1 = &ann->program[G__ANN_PROGRAM_ACTIVATE].inst[0];
	inst2 = &ann->program[G__ANN_PROGRAM_TRAIN].inst[0];
	prefix = capitalize(ann->prefix);

	/* C file */

	if (P(file1,
	      "int %s_version(void) {\n"
	      "  return %d;\n"
	      "}\n\n",
	      ann->prefix,
	      G__VERSION) ||
	    P(file1,
	      "size_t %s_memory_size(void) {\n"
	      "  return %lu;\n"
	      "}\n\n",
	      ann->prefix,
	      UL(ann->precision.size)) ||
	    P(file1,
	      "size_t %s_memory_hard(void) {\n"
	      "  return %lu;\n"
	      "}\n\n",
	      ann->prefix,
	      UL(ann->precision.hard)) ||
	    P(file1,
	      "void %s_initialize(void *m) {\n"
	      "  _initialize_((char *)m);\n"
	      "}\n\n",
	      ann->prefix) ||
	    P(file1,
	      "void *%s_activate(void *m, const void *x) {\n"
	      "  return _activate_((char *)m, (const %s *)x);\n"
	      "}\n\n",
	      ann->prefix,
	      precision(inst1)) ||
	    P(file1,
	      "void %s_train(void *m, const void *x, const void *y) {\n"
	      "  _train_((char *)m, (const %s *)x, (const %s *)y);\n"
	      "}\n\n",
	      ann->prefix,
	      precision(inst2),
	      precision(inst2))) {
		G__FREE(prefix);
		G__DEBUG(0);
		return -1;
	}

	/* H file */

	if (P(file2,
	      "#ifndef _%s_H_\n"
	      "#define _%s_H_\n\n"
	      "#include <stddef.h>\n\n"
	      "#ifdef __cplusplus\n"
	      "extern \"C\" {\n"
	      "#endif /* __cplusplus */\n\n",
	      prefix,
	      prefix) ||
	    P(file2, "int %s_version(void);\n", ann->prefix) ||
	    P(file2, "size_t %s_memory_size(void);\n", ann->prefix) ||
	    P(file2, "size_t %s_memory_hard(void);\n", ann->prefix) ||
	    P(file2, "void %s_initialize(void *m);\n", ann->prefix) ||
	    P(file2,
	      "void *%s_activate(void *m, const void *x);\n",
	      ann->prefix) ||
	    P(file2,
	      "void %s_train(void *m, const void *x, const void *y);\n\n",
	      ann->prefix) ||
	    P(file2,
	      "#ifdef __cplusplus\n"
	      "}\n"
	      "#endif /* __cplusplus */\n\n"
	      "#endif /* _%s_H_ */\n",
	      prefix)) {
		G__FREE(prefix);
		G__DEBUG(0);
		return -1;
	}
	G__FREE(prefix);
	return 0;
}

int
g__emitc(const struct g__ann *ann, const char *tmp)
{
	FILE *file1, *file2;
	size_t n;
	char *s;

	assert( ann );

	n = g__strlen(ann->module) + g__strlen(tmp) + 32;
	s = g__malloc(n);
	if (!s) {
		G__DEBUG(0);
		return -1;
	}
	if (!ann->cuda) {
		g__sprintf(s,
			n,
			"%s%s%s.c",
			g__strlen(tmp) ? tmp : "",
			g__strlen(tmp) ? "/" : "",
			ann->module);
	} else {
		g__sprintf(s,
			n,
			"%s%s%s.cu",
			g__strlen(tmp) ? tmp : "",
			g__strlen(tmp) ? "/" : "",
			ann->module);
	}
	file1 = fopen(s, "w");
	g__sprintf(s,
		   n,
		   "%s%s%s.h",
		   g__strlen(tmp) ? tmp : "",
		   g__strlen(tmp) ? "/" : "",
		   ann->module);
	file2 = fopen(s, "w");
	G__FREE(s);
	if (!file1 || !file2) {
		G__DEBUG(G__ERR_FILE);
		return -1;
	}
	if (header(ann, file1, 1) ||
	    header(ann, file2, 0) ||
	    initialize(ann, file1) ||
	    activate(ann, file1) ||
	    backprop(ann, file1) ||
	    train(ann, file1) ||
	    export(ann, file1, file2)) {
		fclose(file1);
		fclose(file2);
		G__DEBUG(0);
		return -1;
	}
	fclose(file1);
	fclose(file2);
	return 0;
}
