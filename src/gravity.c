/**
 * gravity.c
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

int
main(int argc, char *argv[])
{
	const struct g__ir *ir;
	const char *pathname;
	struct g__ann *ann;
	int i;

	pathname = 0;
	yyerroron = 1;
	g__debug_enabled = 0;
	for (i=1; i<argc; ++i) {
		if (!strcmp("--version", argv[i])) {
			printf("The Gravity Compiler %d.%d\n",
			       G__VERSION / 10,
			       G__VERSION % 10);
		}
		else if (!strcmp("--debug", argv[i])) {
			g__debug_enabled = 1;
		}
		else {
			if (pathname) {
				pathname = 0;
				break;
			}
			pathname = argv[i];
		}
	}
	if (!pathname) {
		printf("usage: gravity [--verion][--debug] input\n");
		return -1;
	}

	/* g compile */

	ir = g__ir_parse(pathname);
	if (!ir) {
		G__DEBUG(0);
		return -1;
	}
	printf("This is a checkpoint.\n");
	printf("The IR structure has batch: %d", ir->batch);
	printf("The IR structure has cuda: %d", ir->cuda);
	ann = g__ann_open(ir);
	g__ir_destroy();
	if (!ann || g__emitc(ann, 0)) {
		g__ann_close(ann);
		fprintf(stderr, "gravity compiler error (run with --debug)\n");
		G__DEBUG(0);
		return -1;
	}
	g__ann_close(ann);
	return 0;
}
