/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* logerr-die.c: _global_err_stream */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */

#include <execinfo.h>

#include <logerr-die.h>

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
