/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* alloc-or-die.h: alloc_or_die malloc_or_die calloc_or_die malloc_or_log */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */

#include "logerr-die.h"

#include <stdlib.h>

void *lffl_calloc_or_die(FILE *log, const char *file, const char *func,
			 int line, const char *var_name, size_t num_members,
			 size_t size_each)
{
	void *p = calloc(num_members, size_each);
	if (!p) {
		lffl_printf(log, file, func, line,
			    "could not calloc(%zu, %zu) (%zu bytes) for %s?",
			    num_members, size_each, num_members * size_each,
			    var_name);
		exit(EXIT_FAILURE);
	}
	return p;
}

void *lffl_malloc_or_log(FILE *log, const char *file, const char *func,
			 int line, const char *var_name, size_t size)
{
	void *p = malloc(size);
	if (!p) {
		lffl_printf(log, file, func, line,
			    "could not malloc(%zu) for %s?", size, var_name);
	}
	return p;
}

void *lffl_malloc_or_die(FILE *log, const char *file, const char *func,
			 int line, const char *var_name, size_t size)
{
	void *p = lffl_malloc_or_log(log, file, func, line, var_name, size);
	if (!p) {
		exit(EXIT_FAILURE);
	}
	return p;
}
