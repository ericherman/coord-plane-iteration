/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* cli-coord-plane-iteration.c: playing with mandlebrot and such */
/* Copyright (C) 2020-2023 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */

#define CLI_COORD_PLANE_ITERATION_VERSION "0.1.0"

#include <signal.h>

#include "logerr-die.h"
#include "coord-plane-option-parser.h"

#ifndef Make_valgrind_happy
#define Make_valgrind_happy 0
#endif

int coord_plane_char_update(coordinate_plane_s *plane, char c)
{
	switch (c) {
	case 'q':
		return 1;
	case 'j':
		coordinate_plane_next_function(plane);
		return 0;
	case 'm':
		coordinate_plane_threads_more(plane);
		return 0;
	case 'n':
		coordinate_plane_threads_less(plane);
		return 0;
	case 'w':
		coordinate_plane_pan_up(plane);
		return 0;
	case 's':
		coordinate_plane_pan_down(plane);
		return 0;
	case 'a':
		coordinate_plane_pan_left(plane);
		return 0;
	case 'd':
		coordinate_plane_pan_right(plane);
		return 0;
	case 'x':
		coordinate_plane_zoom_out(plane);
		return 0;
	case 'z':
		coordinate_plane_zoom_in(plane);
		return 0;
	}

	return 0;
}

void fclear_screen(FILE *out)
{
	fflush(out);
	fprintf(out, "\033[H\033[J");
	fflush(out);
}

void fprint_coordinate_plane_ascii(FILE *out, coordinate_plane_s *plane)
{
	fclear_screen(out);
	uint32_t win_height = coordinate_plane_win_height(plane);
	uint32_t win_width = coordinate_plane_win_width(plane);
	for (size_t y = 0; y < win_height; ++y) {
		for (size_t x = 0; x < win_width; ++x) {
			uint64_t escaped =
			    coordinate_plane_escaped(plane, x, y);
			char c;
			if (escaped == 0) {
				c = ' ';
			} else if (escaped < 10) {
				c = '0' + escaped;
			} else if (escaped >= 10 && escaped < 36) {
				c = 'A' + (escaped - 10);
			} else if (escaped >= 36 && escaped < (36 + 26)) {
				c = 'a' + escaped - 36;
			} else {
				c = '*';
			}
			printf("%c", c);
		}
		fprintf(out, "\n");
	}
}

int main(int argc, char **argv)
{
	signal(SIGSEGV, backtrace_exit_handler);

	const char *version = CLI_COORD_PLANE_ITERATION_VERSION;
	coordinate_plane_s *plane =
	    coordinate_plane_new_from_args(argc, argv, version);

	size_t it_per_frame = 1;

	int halt = 0;
	for (unsigned long i = 0; !halt; ++i) {
		int c = 0;
		coordinate_plane_iterate(plane, it_per_frame);
		fprint_coordinate_plane_ascii(stdout, plane);
		const char *title = coordinate_plane_function_name(plane);
		size_t escaped = coordinate_plane_escaped_count(plane);
		size_t not_escaped = coordinate_plane_not_escaped_count(plane);
		fprintf(stdout, "%s %lu escaped: %zu not: %zu", title, i,
			escaped, not_escaped);
		fflush(stdout);
		if (!coordinate_plane_halt_after(plane)) {
			fprintf(stdout,
				" <enter> to continue, 'q<enter>' to quit: ");
			fflush(stdout);
			c = getchar();
			if (c == 'q') {
				halt = 1;
			}
			coord_plane_char_update(plane, c);
		} else if (i >= coordinate_plane_halt_after(plane)) {
			halt = 1;
		}
	}
	fprintf(stdout, "\n");

	if (Make_valgrind_happy) {
		coordinate_plane_free(plane);
	}
}
