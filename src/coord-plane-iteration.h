/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* coord-plane-iteration.h: playing with mandelbrot and such */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */

#ifndef COORD_PLANE_ITERATION_H
#define COORD_PLANE_ITERATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "basic-thread-pool.h"

/*
 on x86_64:
 sizeof(float):          4 bytes,  32 bits
 sizeof(double):         8 bytes,  64 bits
 sizeof(long double):   16 bytes, 128 bits
*/
struct ldxy {
	long double x;
	long double y;
};

#define pfuncs_mandelbrot_idx	0U
#define pfuncs_julia_idx	(pfuncs_mandelbrot_idx + 1U)
extern const size_t pfuncs_len;

struct coordinate_plane;

struct coordinate_plane *coordinate_plane_reset(struct coordinate_plane *plane,
						uint32_t win_width,
						uint32_t win_height,
						struct ldxy center,
						long double resolution_x,
						long double resolution_y,
						size_t pfuncs_idx,
						struct ldxy seed);

struct coordinate_plane *coordinate_plane_new(const char *program_name,
					      uint32_t win_width,
					      uint32_t win_height,
					      struct ldxy center,
					      long double resolution_x,
					      long double resolution_y,
					      size_t pfunc_idx,
					      struct ldxy seed,
					      uint64_t halt_after,
					      uint32_t skip_rounds,
					      uint32_t num_threads);

void coordinate_plane_free(struct coordinate_plane *plane);

void coordinate_plane_resize(struct coordinate_plane *plane,
			     uint32_t new_win_width, uint32_t new_win_height,
			     bool preserve_ratio);

size_t coordinate_plane_iterate(struct coordinate_plane *plane, uint32_t steps);

void coordinate_plane_next_function(struct coordinate_plane *plane);

void coordinate_plane_zoom_in(struct coordinate_plane *plane);

void coordinate_plane_zoom_out(struct coordinate_plane *plane);

void coordinate_plane_pan_left(struct coordinate_plane *plane);

void coordinate_plane_pan_right(struct coordinate_plane *plane);

void coordinate_plane_pan_up(struct coordinate_plane *plane);

void coordinate_plane_pan_down(struct coordinate_plane *plane);

void coordinate_plane_recenter(struct coordinate_plane *plane,
			       uint32_t x, uint32_t y);

void coordinate_plane_threads_more(struct coordinate_plane *plane);

void coordinate_plane_threads_less(struct coordinate_plane *plane);

uint64_t coordinate_plane_escaped(struct coordinate_plane *plane, uint32_t x,
				  uint32_t y);
uint64_t coordinate_plane_iteration_count(struct coordinate_plane *plane);

uint32_t coordinate_plane_win_width(struct coordinate_plane *plane);
uint32_t coordinate_plane_win_height(struct coordinate_plane *plane);
long double coordinate_plane_x_min(struct coordinate_plane *plane);
long double coordinate_plane_y_min(struct coordinate_plane *plane);
long double coordinate_plane_x_max(struct coordinate_plane *plane);
long double coordinate_plane_y_max(struct coordinate_plane *plane);
const char *coordinate_plane_program(struct coordinate_plane *plane);
const char *coordinate_plane_function_name(struct coordinate_plane *plane);
size_t coordinate_plane_function_index(struct coordinate_plane *plane);
void coordinate_plane_center(struct coordinate_plane *plane, struct ldxy *out);
void coordinate_plane_seed(struct coordinate_plane *plane, struct ldxy *out);
long double coordinate_plane_resolution_x(struct coordinate_plane *plane);
long double coordinate_plane_resolution_y(struct coordinate_plane *plane);
uint64_t coordinate_plane_halt_after(struct coordinate_plane *plane);
uint32_t coordinate_plane_skip_rounds(struct coordinate_plane *plane);
size_t coordinate_plane_escaped_count(struct coordinate_plane *plane);
size_t coordinate_plane_not_escaped_count(struct coordinate_plane *plane);
size_t coordinate_plane_trapped_count(struct coordinate_plane *plane);
size_t coordinate_plane_unchanged(struct coordinate_plane *plane);
size_t coordinate_plane_num_threads(struct coordinate_plane *plane);

#ifndef SKIP_THREADS
struct basic_thread_pool *coordinate_plane_thread_pool(struct coordinate_plane
						       *plane);
#endif

#endif /* COORD_PLANE_ITERATION_H */
