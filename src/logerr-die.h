/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* logerr-die.h: die(fmt, ...), logerr(fmt, ...) */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#ifndef LOGERR_DIE_H
#define LOGERR_DIE_H 1

#include <stdio.h>		/* FILE, fprintf */

extern FILE *_global_err_stream;

void backtrace_exit_handler(int sig);

void lffl_printf(FILE *log, const char *file, const char *func, int line,
		 const char *format, ...);

void lffl_die(FILE *log, const char *file, const char *func, int line,
	      const char *format, ...);

#ifndef _err_stream
#define _err_stream (_global_err_stream ? _global_err_stream : stderr)
#endif

#ifndef errorf
#define errorf(format, ...) \
	lffl_printf(_err_stream, __FILE__, __func__, __LINE__, \
		    format __VA_OPT__(,) __VA_ARGS__)
#endif

#ifndef die
#define die(format, ...) \
	lffl_die(_err_stream, __FILE__, __func__, __LINE__, \
		 format __VA_OPT__(,) __VA_ARGS__)
#endif

#endif /* LOGERR_DIE_H */
