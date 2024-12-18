/**
 * g_vcm.c
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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include "g_common.h"
#include "g_vcm.h"

struct g__vcm {
	void *handle;
};

static int
compile(const char *input, const char *output)
{
	char *file, *argv[15];
	int status;
	pid_t pid;

	pid = fork();
	if (0 > pid) {
		G__DEBUG(G__ERR_SYSTEM);
		return -1;
	}
	if (!pid) {
		file = g__strlen(getenv("CC")) ? getenv("CC") : "/usr/bin/cc";
		argv[ 0] = "cc";
		argv[ 1] = "-ansi";
		argv[ 2] = "-pedantic";
		argv[ 3] = "-Wshadow";
		argv[ 4] = "-Wall";
		argv[ 5] = "-Wextra";
		argv[ 6] = "-Werror";
		argv[ 7] = "-Wfatal-errors";
		argv[ 8] = "-fPIC";
		argv[ 9] = "-O3";
		argv[10] = "-shared";
		argv[11] = (char *)input;
		argv[12] = "-o";
		argv[13] = (char *)output;
		argv[14] = 0;
		if (0 > execvp(file, argv)) {
			G__DEBUG(G__ERR_SYSTEM);
			return -1;
		}
	}
	else {
		status = 0;
		while (pid != waitpid(pid, &status, 0));
		if (status) {
			G__DEBUG(G__ERR_JITC);
			return -1;
		}
	}
	return 0;
}

g__vcm_t
g__vcm_open(const char *pathname)
{
	struct g__vcm *vcm;
	const char *tmp;
	size_t n;
	char *s;

	assert( g__strlen(pathname) );

	tmp = getenv("TMPDIR");
	tmp = tmp ? tmp : getenv("TMP");
	tmp = tmp ? tmp : getenv("TEMP");
	tmp = tmp ? tmp : ".";
	n = g__strlen(tmp) + 32;
	s = g__malloc(n);
	if (!s) {
		G__DEBUG(0);
		return 0;
	}
	g__sprintf(s, n, "%s/_%x_.so", tmp, rand());
	if (compile(pathname, s)) {
		G__FREE(s);
		G__DEBUG(0);
		return 0;
	}
	vcm = g__malloc(sizeof (struct g__vcm));
	if (!vcm) {
		g__unlink(s);
		G__FREE(s);
		G__DEBUG(0);
		return 0;
	}
	memset(vcm, 0, sizeof (struct g__vcm));
	vcm->handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL);
	g__unlink(s);
	G__FREE(s);
	if (!vcm->handle) {
		g__vcm_close(vcm);
		G__DEBUG(G__ERR_SYSTEM);
		return 0;
	}
	return vcm;
}

void
g__vcm_close(g__vcm_t vcm)
{
	if (vcm && vcm->handle) {
		dlclose(vcm->handle);
	}
	G__FREE(vcm);
}

long
g__vcm_lookup(g__vcm_t vcm, const char *symbol)
{
	assert( vcm && g__strlen(symbol) );

	return (long)dlsym(vcm->handle, symbol);
}
