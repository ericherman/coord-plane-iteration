/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* logerr-die.c: _global_err_stream */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#include <execinfo.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>

#include "logerr-die.h"

FILE *asynch_unsafe_global_err_stream = NULL;

FILE *logger_set_global_err_stream(FILE *stream)
{
	FILE *was = asynch_unsafe_global_err_stream;
	asynch_unsafe_global_err_stream = stream;
	return was;
}

FILE *logger_get_global_err_stream(void)
{
	return asynch_unsafe_global_err_stream ? asynch_unsafe_global_err_stream
	    : stderr;
}

/* this is wildly asynch-unsafe, but we're already crashing, maybe get info */
static void asynch_unsafe_backtrace_exit_handler(int sig)
{
	void *array[100];
	size_t size;
	FILE *log;

	size = backtrace(array, 100);
	log = logger_get_global_err_stream();

	if (log == stderr) {
		fflush(stdout);
	}
	fprintf(log, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, fileno(log));
	fflush(log);
	exit(EXIT_FAILURE);
}

void pray_for_debug_info_on_segfault(void)
{
	signal(SIGSEGV, asynch_unsafe_backtrace_exit_handler);
}

static void lffl_vprintf(FILE *log, const char *file, const char *func,
			 int line, const char *format, va_list ap)
{
	/* avoid POSIX undefined stdout+stderr */
	/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_05_01 */
	if (log == stderr) {
		fflush(stdout);
	}

	fprintf(log, "%s:%s():%d: ", file, func, line);
	vfprintf(log, format, ap);
	fprintf(log, "\n");
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
