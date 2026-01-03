/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* alloc-or-die.h: */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#ifndef ALLOC_OR_DIE_H
#define ALLOC_OR_DIE_H 1

#include <stddef.h>		/* size_t */

#include "logerr-die.h"

void *lffl_calloc_or_die(FILE *log, const char *file, const char *func,
			 int line, const char *var_name, size_t num_members,
			 size_t size_each);
#ifndef calloc_or_die
#define calloc_or_die(var_name, num_members, size_each) \
	lffl_calloc_or_die(_err_stream, __FILE__, __func__, __LINE__, \
		var_name, num_members, size_each)
#endif

void *lffl_malloc_or_die(FILE *log, const char *file, const char *func,
			 int line, const char *var_name, size_t size);
#ifndef malloc_or_die
#define malloc_or_die(var_name, size) \
	lffl_malloc_or_die(_err_stream, __FILE__, __func__, __LINE__, \
			   var_name, size)
#endif

void *lffl_malloc_or_log(FILE *log, const char *file, const char *func,
			 int line, const char *var_name, size_t size);
#ifndef malloc_or_log
#define malloc_or_log(var_name, size) \
	lffl_malloc_or_log(_err_stream, __FILE__, __func__, __LINE__, \
			var_name, size)
#endif

#ifndef alloc_or_die
#ifndef NDEBUG
#define alloc_or_die(var_name, size) calloc_or_die(var_name, 1, size)
#else
#define alloc_or_die(var_name, size) malloc_or_die(var_name, size)
#endif
#endif

#endif /* ALLOC_OR_DIE_H */
