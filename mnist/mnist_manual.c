/**
 * mnist.c
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

#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../src/test.h"

#define BATCH  8
#define EPOCHS 4
#define REAL_T float

#define MKSTRING(x) #x
#define TOSTRING(x) MKSTRING(x)
#define UL(x)       ((unsigned long)(x))

typedef REAL_T real_t;

static uint64_t
usec(void)
{
	struct timeval t;

	gettimeofday(&t, 0);
	return (uint64_t)t.tv_usec + (uint64_t)t.tv_sec * 1000000;
}

static int32_t
swap(int32_t x)
{
	union { int32_t i; char b[4]; } in, out;

	in.i = x;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	return out.i;
}

static int
argmax(const real_t *a, int n)
{
	real_t max;
	int i, j;

	max = a[0];
	for (i=j=0; i<n; ++i) {
		if (max < a[i]) {
			max = a[i];
			j = i;
		}
	}
	return j;
}

static int
train_and_test(void *alloc_mem,
	       const uint8_t *train_y,
	       const uint8_t *train_x,
	       const uint8_t *test_y,
	       const uint8_t *test_x,
	       int train_n,
	       int test_n)
{
	const uint8_t *labels, *images;
	int i, j, k, m, error;
	real_t *x, *y, *z;
	uint64_t t[3];

	x = (real_t *)malloc(BATCH * 28 * 28 * sizeof (x[0]));
	y = (real_t *)malloc(BATCH * 10 * sizeof (y[0]));
	if (!x || !y) {
		free(x);
		free(y);
		fprintf(stderr, "out of memory\n");
		return -1;
	}
    srand(10);
	/* train */

	t[0] = usec();
	m = train_n / BATCH;
	labels = train_y;
	images = train_x;
	for (i=0; i<m; ++i) {
		for (j=0; j<BATCH; ++j) {
			for (k=0; k<(28*28); ++k) {
				x[j * (28*28) + k] = (*images++) / 255.0;
			}
			for (k=0; k<10; ++k) {
				y[j * 10 + k] = 0.0;
			}
			y[j * 10 + (*labels++)] = 1.0;
		}
		test_train(alloc_mem, x, y);
		printf("\r%06d/%06d", i, m);
		fflush(stdout);
	}
	t[1] = usec() - t[0];
	printf("\rtrain done (%.2f sec)\n", t[1] * 1e-6);

	/* test */

	t[0] = usec();
	error = 0;
	labels = test_y;
	images = test_x;
	for (i=0; i<test_n; ++i) {
		for (k=0; k<(28*28); ++k) {
			x[k] = (*images++) / 255.0;
		}
		z = test_activate(alloc_mem, x);
		if (argmax(z, 10) != (int)(*labels++)) {
			error++;
		}
		printf("\r%06d/%06d", i, test_n);
		fflush(stdout);
	}
	t[2] = usec() - t[0];
	printf("\rtest done (%.2f sec, %d errors)\n", t[2] * 1e-6, error);

	printf("Train Time: %.0f usec/sample\n", (double)t[1] / train_n);
	printf("Test  Time: %.0f usec/sample\n", (double)t[2] / train_n);
	printf("Accuracy  : %.4f\n", 1.0 - ((double)error / test_n));

	/* done */

	free(x);
	free(y);
	return 0;
}

static uint8_t *
load_labels(const char *pathname, int *n)
{
	int32_t meta[2];
	uint8_t *data;
	FILE *file;

	file = fopen(pathname, "r");
	if (!file) {
		fprintf(stderr, "unable to open file\n");
		return 0;
	}
	if (sizeof (meta) != fread(meta, 1, sizeof (meta), file)) {
		fclose(file);
		fprintf(stderr, "unable to read file\n");
		return 0;
	}
	if ((0x1080000 != meta[0]) || (0 >= swap(meta[1]))) {
		fclose(file);
		fprintf(stderr, "invalid file\n");
		return 0;
	}
	(*n) = swap(meta[1]);
	meta[1] = (*n);
	data = (uint8_t *)malloc(meta[1]);
	if (!data) {
		fclose(file);
		fprintf(stderr, "out of memory\n");
		return 0;
	}
	if ((size_t)meta[1] != fread(data, 1, meta[1], file)) {
		free(data);
		fclose(file);
		fprintf(stderr, "unable to read file\n");
		return 0;
	}
	fclose(file);
	return data;
}

static uint8_t *
load_images(const char *pathname, int *n)
{
	int32_t meta[4];
	uint8_t *data;
	FILE *file;

	file = fopen(pathname, "r");
	if (!file) {
		fprintf(stderr, "unable to open file\n");
		return 0;
	}
	if (sizeof (meta) != fread(meta, 1, sizeof (meta), file)) {
		fclose(file);
		fprintf(stderr, "unable to read file\n");
		return 0;
	}
	if ((0x3080000 != meta[0]) ||
	    (0  >= swap(meta[1])) ||
	    (28 != swap(meta[2])) ||
	    (28 != swap(meta[3]))) {
		fclose(file);
		fprintf(stderr, "invalid file\n");
		return 0;
	}
	(*n) = swap(meta[1]);
	meta[1] = (*n) * 28 * 28;
	data = (uint8_t *)malloc(meta[1]);
	if (!data) {
		fclose(file);
		fprintf(stderr, "out of memory\n");
		return 0;
	}
	if ((size_t)meta[1] != fread(data, 1, meta[1], file)) {
		free(data);
		fclose(file);
		fprintf(stderr, "unable to read file\n");
		return 0;
	}
	fclose(file);
	return data;
}

int
main()
{
	int train_y_n, train_x_n, test_y_n, test_x_n;
	uint8_t *train_y, *train_x, *test_y, *test_x;
	int i, e;
    void* alloc_mem;

	/* open ANN */

	alloc_mem = malloc(test_memory_size());
	if (!alloc_mem) {
		printf("Error with alloc_mem!");
		return -1;
	}
	memset(alloc_mem, 0, test_memory_size());

    /* populate */

    test_initialize(alloc_mem);

	printf("version: %d\n", test_version());
	printf("size   : %lu\n", UL(test_memory_size()));
	printf("hard   : %lu\n", UL(test_memory_hard()));

	/* load train/test data */

	e = 0;
	train_y = load_labels("data/train-labels", &train_y_n);
	train_x = load_images("data/train-images", &train_x_n);
	test_y = load_labels("data/test-labels", &test_y_n);
	test_x = load_images("data/test-images", &test_x_n);
	if (!train_y ||
	    !train_x ||
	    !test_y ||
	    !test_x ||
	    (train_y_n != train_x_n) ||
	    (test_y_n != test_x_n)) {
		e = -1;
		fprintf(stderr, "failed to load valid train/test data\n");
	}

	/* train and test */

	for (i=0; i<EPOCHS; ++i) {
		printf("--- EPOCH %d ---\n", i);
		if (!e) {
            srand(10);
			if (train_and_test(alloc_mem,
					   train_y,
					   train_x,
					   test_y,
					   test_x,
					   train_y_n,
					   test_y_n)) {
				e = -1;
				fprintf(stderr, "failed to train/test\n");
			}
		}
	}

	/* close */

    free(alloc_mem);
	free(train_y);
	free(train_x);
	free(test_y);
	free(test_x);
	return e;
}
