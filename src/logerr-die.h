/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* logerr-die.h: die(fmt, ...), logerr(fmt, ...) */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */

#ifndef LOGERR_DIE_H
#define LOGERR_DIE_H 1

#include <stdio.h>		/* FILE, fprintf */
#include <stdlib.h>		/* exit(EXIT_FAILURE) */

extern FILE *_global_err_stream;

#ifndef _err_stream
#define _err_stream (_global_err_stream ? _global_err_stream : stderr)
#endif

#ifndef logerror
#define logerror(format, ...) \
	do { \
		fflush(stdout); /* avoid POSIX undefined stdout+stderr */ \
		/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_05_01 */ \
		fprintf(_err_stream, "%s:%d: ", __FILE__, __LINE__); \
		/* fprintf(_err_stream, format __VA_OPT__(,) __VA_ARGS__); */ \
		/* fprintf(_err_stream, format, ##_VA_ARGS__); */ \
		fprintf(_err_stream, format, __VA_ARGS__); \
		fprintf(_err_stream, "\n"); \
	} while (0)
#endif

#ifndef die
#define die(format, ...) \
	do { \
		logerror(format, __VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while (0)
#endif

void backtrace_exit_handler(int sig);

#endif /* LOGERR_DIE_H */
