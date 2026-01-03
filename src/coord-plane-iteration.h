/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* coord-plane-iteration.h: playing with mandlebrot and such */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */

#ifndef COORD_PLANE_ITERATION_H
#define COORD_PLANE_ITERATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 on x86_64:
 sizeof(float):          4 bytes,  32 bits
 sizeof(double):         8 bytes,  64 bits
 sizeof(long double):   16 bytes, 128 bits
*/
typedef struct ldxy {
	long double x;
	long double y;
} ldxy_s;

typedef struct iterxy {
	ldxy_s seed;

	/* coordinate location */
	ldxy_s c;

	/* calculated next location */
	ldxy_s z;

	uint32_t escaped;
} iterxy_s;

typedef void (*pfunc_init_f)(iterxy_s *p, ldxy_s xy, ldxy_s seed);
typedef void (*pfunc_f)(iterxy_s *p);
typedef bool (*pfunc_escape_f)(ldxy_s xy);

typedef struct named_pfunc {
	pfunc_init_f pfunc_init;
	pfunc_escape_f pfunc_escape;
	pfunc_f pfunc;
	const char *name;
} named_pfunc_s;

#define pfuncs_mandlebrot_idx	0U
#define pfuncs_julia_idx	(pfuncs_mandlebrot_idx + 1U)
extern named_pfunc_s pfuncs[];
extern const size_t pfuncs_len;

struct coordinate_plane;
typedef struct coordinate_plane coordinate_plane_s;

coordinate_plane_s *coordinate_plane_reset(coordinate_plane_s *plane,
					   uint32_t win_width,
					   uint32_t win_height,
					   ldxy_s center,
					   long double resolution_x,
					   long double resolution_y,
					   size_t pfuncs_idx, ldxy_s seed);

coordinate_plane_s *coordinate_plane_new(const char *program_name,
					 uint32_t win_width,
					 uint32_t win_height,
					 ldxy_s center,
					 long double resolution_x,
					 long double resolution_y,
					 size_t pfunc_idx,
					 ldxy_s seed,
					 uint64_t halt_after,
					 uint32_t skip_rounds,
					 uint32_t num_threads);

void coordinate_plane_free(coordinate_plane_s *plane);

void coordinate_plane_resize(coordinate_plane_s *plane, uint32_t new_win_width,
			     uint32_t new_win_height, bool preserve_ratio);

size_t coordinate_plane_iterate(coordinate_plane_s *plane, uint32_t steps);

void coordinate_plane_next_function(coordinate_plane_s *plane);

void coordinate_plane_zoom_in(coordinate_plane_s *plane);

void coordinate_plane_zoom_out(coordinate_plane_s *plane);

void coordinate_plane_pan_left(coordinate_plane_s *plane);

void coordinate_plane_pan_right(coordinate_plane_s *plane);

void coordinate_plane_pan_up(coordinate_plane_s *plane);

void coordinate_plane_pan_down(coordinate_plane_s *plane);

void coordinate_plane_recenter(coordinate_plane_s *plane,
			       uint32_t x, uint32_t y);

void coordinate_plane_threads_more(coordinate_plane_s *plane);

void coordinate_plane_threads_less(coordinate_plane_s *plane);

uint64_t coordinate_plane_escaped(coordinate_plane_s *plane, uint32_t x,
				  uint32_t y);
uint64_t coordinate_plane_iteration_count(coordinate_plane_s *plane);

uint32_t coordinate_plane_win_width(coordinate_plane_s *plane);
uint32_t coordinate_plane_win_height(coordinate_plane_s *plane);
long double coordinate_plane_x_min(coordinate_plane_s *plane);
long double coordinate_plane_y_min(coordinate_plane_s *plane);
long double coordinate_plane_x_max(coordinate_plane_s *plane);
long double coordinate_plane_y_max(coordinate_plane_s *plane);
const char *coordinate_plane_program(coordinate_plane_s *plane);
const char *coordinate_plane_function_name(coordinate_plane_s *plane);
size_t coordinate_plane_function_index(coordinate_plane_s *plane);
void coordinate_plane_center(coordinate_plane_s *plane, ldxy_s *out);
void coordinate_plane_seed(coordinate_plane_s *plane, ldxy_s *out);
long double coordinate_plane_resolution_x(coordinate_plane_s *plane);
long double coordinate_plane_resolution_y(coordinate_plane_s *plane);
uint64_t coordinate_plane_halt_after(coordinate_plane_s *plane);
uint32_t coordinate_plane_skip_rounds(coordinate_plane_s *plane);
size_t coordinate_plane_escaped_count(coordinate_plane_s *plane);
size_t coordinate_plane_not_escaped_count(coordinate_plane_s *plane);
size_t coordinate_plane_num_threads(coordinate_plane_s *plane);

#endif /* COORD_PLANE_ITERATION_H */
