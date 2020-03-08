/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* alloc-or-die.h: alloc_or_die malloc_or_die calloc_or_die malloc_or_log */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */

#ifndef ALLOC_OR_DIE_H
#define ALLOC_OR_DIE_H 1

#include <stddef.h>		/* size_t */
#include <stdlib.h>		/* malloc, calloc */

#include <logerr-die.h>

#ifndef calloc_or_die
#define calloc_or_die(pptr, n_members, size_each) \
	do { \
		size_t _codie_nmeb = (n_members); \
		size_t _codie_size = (size_each); \
		void *_codie_ptr = *(pptr) = calloc(_codie_nmeb, _codie_size); \
		if (!_codie_ptr) { \
			size_t _codie_tot = (_codie_nmeb * _codie_size); \
			die("could not calloc(%zu, %zu) (%zu bytes) for %s?", \
				_codie_nmeb, _codie_size, _codie_tot, #pptr); \
		} \
	} while (0)
#endif

#ifndef malloc_or_die
#define malloc_or_die(pptr, size) \
	do { \
		size_t _modie_size = (size); \
		void * _modie_ptr = *(pptr) = malloc(_modie_size); \
		if (!_modie_ptr) { \
			die("could not allocate %zu bytes for %s?", \
				_modie_size, #pptr); \
		} \
	} while (0)
#endif

#ifndef malloc_or_log
#define malloc_or_log(pptr, size) \
	do { \
		size_t _modie_size = (size); \
		void * _modie_ptr = *(pptr) = malloc(_modie_size); \
		if (!_modie_ptr) { \
			logerror("could not allocate %zu bytes for %s?", \
				_modie_size, #pptr); \
		} \
	} while (0)
#endif

#ifndef alloc_or_die
#ifndef NDEBUG
#define alloc_or_die(pptr, size) calloc_or_die(pptr, 1, size)
#else
#define alloc_or_die(pptr, size) malloc_or_die(pptr, size)
#endif
#endif

#endif /* ALLOC_OR_DIE_H */
