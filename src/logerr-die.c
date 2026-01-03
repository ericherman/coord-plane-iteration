/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* logerr-die.c: _global_err_stream */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#include <execinfo.h>
#include <stdarg.h>
#include <stdlib.h>

#include "logerr-die.h"

FILE *_global_err_stream = NULL;

void backtrace_exit_handler(int sig)
{
	void *array[100];
	size_t size;

	size = backtrace(array, 100);

	fprintf(_err_stream, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, fileno(_err_stream));
	exit(EXIT_FAILURE);
}

void lffl_vprintf(FILE *log, const char *file, const char *func, int line,
		  const char *format, va_list ap)
{
	/* avoid POSIX undefined stdout+stderr */
	/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_05_01 */
	if (log == stderr) {
		fflush(stdout);
	}

	fprintf(log, "%s:%s():%d: ", file, func, line);
	vfprintf(log, format, ap);
	fprintf(_err_stream, "\n");
}

void lffl_printf(FILE *log, const char *file, const char *func, int line,
		 const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	lffl_vprintf(log, file, func, line, format, ap);
	va_end(ap);
}

void lffl_die(FILE *log, const char *file, const char *func, int line,
	      const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	lffl_vprintf(log, file, func, line, format, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}
