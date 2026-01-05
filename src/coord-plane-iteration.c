/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* coord-plane-iteration.c: playing with mandlebrot and such */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef SKIP_THREADS
#include <stdatomic.h>
#include "basic-thread-pool.h"
#endif

#include "alloc-or-die.h"
#include "coord-plane-iteration.h"

/* the y is understood to contain an i, the sqrt(-1) */
static void square_complex(ldxy_s *out, ldxy_s in)
{
	assert(out);

	/* generate and combine together the four combos */
	long double xx = in.x * in.x;	/* no i */
	long double yx = in.y * in.x;	/* has i */
	long double xy = in.x * in.y;	/* has i */
	long double yy = in.y * in.y * -1;	/* loses the i */

	out->x = xx + yy;	/* terms do not contain i */
	out->y = yx + xy;	/* terms contain an i */
}

static void iterxy_init_zero(iterxy_s *p, ldxy_s xy, ldxy_s seed)
{
	p->seed = seed;
	p->c.x = xy.x;
	p->c.y = xy.y;
	p->z.x = 0.0;
	p->z.y = 0.0;
	p->escaped = 0;
	p->trapped = 0;
}

static void iterxy_init_xy(iterxy_s *p, ldxy_s xy, ldxy_s seed)
{
	p->seed = seed;
	p->c.x = xy.x;
	p->c.y = xy.y;
	p->z.x = xy.x;
	p->z.y = xy.y;
	p->escaped = 0;
	p->trapped = 0;
}

static long double radius_squared(ldxy_s c)
{
	return ((c.x * c.x) + (c.y * c.y));
}

static bool xy_radius_greater_than_2(ldxy_s xy)
{
	long double escape_radius_squared = (2.0 * 2.0);
	return (radius_squared(xy) > escape_radius_squared) ? true : false;
}

static inline int mandelbrot_in_main_cardioid(ldxy_s c)
{
	long double x = c.x;
	long double y = c.y;

	long double xm = x - 0.25L;
	long double y2 = y * y;
	long double q = xm * xm + y2;

	return q * (q + xm) < 0.25L * y2;
}

static inline int mandelbrot_in_period2_bulb(ldxy_s c)
{
	long double x = c.x + 1.0L;
	long double y = c.y;

	return (x * x + y * y) < 0.0625L;
}

bool mandelbrot_trapped(ldxy_s xy)
{
	if (mandelbrot_in_main_cardioid(xy)) {
		return true;
	}
	if (mandelbrot_in_period2_bulb(xy)) {
		return true;
	}
	return false;
}

static void iterxy_init_zero_mandlebrot_trapped(iterxy_s *p, ldxy_s xy,
						ldxy_s seed)
{
	iterxy_init_zero(p, xy, seed);
	if (mandelbrot_trapped(p->c)) {
		p->trapped = 1;
	}
}

/* Z[n+1] = (Z[n])^2 + Orig */
void mandlebrot(iterxy_s *p)
{
	/* first, square the complex */
	ldxy_s result;
	square_complex(&result, p->z);

	/* then add the original C to the Z[n]^2 result */
	p->z.x = result.x + p->c.x;
	p->z.y = result.y + p->c.y;
}

void julia(iterxy_s *p)
{
	/* first, square the complex */
	ldxy_s result;
	square_complex(&result, p->z);

	/* then add the seed C to the Z[n]^2 result */
	p->z.x = result.x + p->seed.x;
	p->z.y = result.y + p->seed.y;
}

#ifdef INCLUDE_ALL_FUNCTIONS
void ordinary_square(iterxy_s *p)
{
	p->z.y = p->z.y * p->z.y;
	p->z.x = p->z.x * p->z.x;
}

/* Z[n+1] = collapse_to_y2_to_y((Z[n])^2) + Orig */
void square_binomial_collapse_y2_add_orig(iterxy_s *p)
{
	/* z[n+1] = z[n]^2 + B */

	/* squaring a binomial == adding together four combos */
	long double xx = p->z.x * p->z.x;
	long double yx = p->z.y * p->z.x;
	long double xy = p->z.x * p->z.y;
	long double yy = p->z.y * p->z.y;

	long double binomial_x = xx;	/* no y terms */
	long double collapse_y_and_y2_terms = yx + xy + yy;

	/* now add the original C */
	p->z.x = binomial_x + p->c.x;
	p->z.y = collapse_y_and_y2_terms + p->c.y;
}

/* Z[n+1] = ignore_y2((Z[n])^2) + Orig */
void square_binomial_ignore_y2_add_orig(iterxy_s *p)
{
	/* z[n+1] = z[n]^2 + B */

	/* squaring a binomial == adding together four combos */
	long double xx = p->z.x * p->z.x;
	long double yx = p->z.y * p->z.x;
	long double xy = p->z.x * p->z.y;
	/*
	   long double yy = p->z.y * p->z.y;
	 */

	/* now add the original C */
	p->z.x = xx + p->c.x;
	p->z.y = xy + yx + p->c.y;
}

void not_a_circle(iterxy_s *p)
{
	long double xx = p->z.x * p->z.x;
	long double yy = p->z.y * p->z.y;

	p->z.y = yy + (0.5 * p->z.x);
	p->z.x = xx + (0.5 * p->z.y);
}
#endif /* INCLUDE_ALL_FUNCTIONS */

named_pfunc_s pfuncs[] = {
	{ iterxy_init_zero_mandlebrot_trapped, xy_radius_greater_than_2,
	 mandlebrot, "mandlebrot" },
	{ iterxy_init_xy, xy_radius_greater_than_2, julia, "julia" },
#ifdef INCLUDE_ALL_FUNCTIONS
	{ iterxy_init_xy, xy_radius_greater_than_2, ordinary_square,
	 "ordinary_square" },
	{ iterxy_init_xy, xy_radius_greater_than_2, not_a_circle,
	 "not_a_circle" },
	{ iterxy_init_zero, xy_radius_greater_than_2,
	 square_binomial_collapse_y2_add_orig,
	 "square_binomial_collapse_y2_add_orig," },
	{ iterxy_init_zero, xy_radius_greater_than_2,
	 square_binomial_ignore_y2_add_orig,
	 "square_binomial_ignore_y2_add_orig," }
#endif /* INCLUDE_ALL_FUNCTIONS */
};

#ifdef INCLUDE_ALL_FUNCTIONS
const size_t pfuncs_len = 5;
#else
const size_t pfuncs_len = 2;
#endif /* INCLUDE_ALL_FUNCTIONS */

struct coordinate_plane_iterate_context;
typedef struct coordinate_plane_iterate_context
    coordinate_plane_iterate_context_s;

struct coordinate_plane_iterate_context {
	coordinate_plane_s *plane;
	size_t steps;
	size_t offset;
	size_t step_size;
	size_t local_escaped;
	size_t local_not_escaped;
	iterxy_s **not_escaped;
	size_t not_escaped_len;
#ifndef SKIP_THREADS
	atomic_bool done;
#else
	bool done;
#endif
};

long double coordinate_plane_x_min(coordinate_plane_s *plane)
{
	return plane->center.x - (plane->resolution_x * (plane->win_width / 2));
}

long double coordinate_plane_y_min(coordinate_plane_s *plane)
{
	return plane->center.y -
	    (plane->resolution_y * (plane->win_height / 2));
}

long double coordinate_plane_x_max(coordinate_plane_s *plane)
{
	return plane->center.x + (plane->resolution_x * (plane->win_width / 2));
}

long double coordinate_plane_y_max(coordinate_plane_s *plane)
{
	return plane->center.y +
	    (plane->resolution_y * (plane->win_height / 2));
}

coordinate_plane_s *coordinate_plane_reset(coordinate_plane_s *plane,
					   uint32_t win_width,
					   uint32_t win_height,
					   ldxy_s center,
					   long double resolution_x,
					   long double resolution_y,
					   size_t pfuncs_idx, ldxy_s seed)
{
	plane->win_width = win_width;
	plane->win_height = win_height;
	plane->center = center;
	plane->resolution_x = resolution_x;
	plane->resolution_y = resolution_y;
	if (!(plane->resolution_x > 0.0)) {
		die("invalid resolution_x %.*Lg", DECIMAL_DIG, resolution_x);
	}
	if (!(plane->resolution_y > 0.0)) {
		die("invalid resolution_y %.*Lg", DECIMAL_DIG, resolution_y);
	}
	plane->iteration_count = 0;
	plane->escaped = 0;
	plane->trapped = 0;
	plane->not_escaped = 0;
	plane->pfuncs_idx = pfuncs_idx;
	/* cache the seed on the plane for reset */
	plane->seed = seed;

	size_t needed = win_width * win_height;
	if (plane->all_points && (plane->all_points_len < needed)) {
		free(plane->all_points);
		plane->all_points = NULL;
		plane->all_points_len = 0;

		free(plane->scratch);
		plane->scratch = NULL;
		plane->scratch_len = 0;

		free(plane->points_not_escaped);
		plane->points_not_escaped = NULL;
		plane->points_not_escaped_len = 0;
	}
	if (!plane->all_points) {
		size_t size = needed * sizeof(iterxy_s);
		plane->all_points = malloc_or_die("plane->all_points", size);
		plane->all_points_len = needed;

		size = needed * sizeof(iterxy_s *);
		plane->scratch = malloc_or_die("plane->scratch", size);
		plane->scratch_len = needed;

		size = needed * sizeof(iterxy_s *);
		plane->points_not_escaped =
		    malloc_or_die("plane->points_not_escaped", size);
		plane->points_not_escaped_len = needed;
	}

	pfunc_init_f pfunc_init = pfuncs[plane->pfuncs_idx].pfunc_init;
	long double x_min = coordinate_plane_x_min(plane);
	long double y_max = coordinate_plane_y_max(plane);
	for (size_t py = 0; py < plane->win_height; ++py) {
		for (size_t px = 0; px < plane->win_width; ++px) {
			size_t i = (py * plane->win_width) + px;

			iterxy_s *p = plane->all_points + i;

			/* location on the co-ordinate plane */
			ldxy_s xy;
			xy.y = y_max - (py * plane->resolution_y);
			if (fabsl(xy.y) < (plane->resolution_y / 2)) {
				/* near enought to zero to call it zero */
				xy.y = 0.0;
			}

			xy.x = x_min + px * plane->resolution_x;
			if (fabsl(xy.x) < (plane->resolution_x / 2)) {
				/* near enought to zero to call it zero */
				xy.x = 0.0;
			}

			pfunc_init(p, xy, seed);
			if (p->trapped) {
				++plane->trapped;
			} else {
				plane->points_not_escaped[plane->not_escaped] =
				    p;
				++(plane->not_escaped);
			}
		}
	}
	plane->unchanged = 0;
	return plane;
}

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
					 uint32_t num_threads)
{
	coordinate_plane_s *plane = NULL;
	size_t size = sizeof(coordinate_plane_s);

	plane = malloc_or_die("plane", size);
	memset(plane, 0x00, size);

	plane->argv0 = program_name;
	plane->halt_after = halt_after;
	plane->skip_rounds = skip_rounds;

	coordinate_plane_iterate_context_s *contexts;
	plane->contexts_len = num_threads ? num_threads : 1;
	size = sizeof(coordinate_plane_iterate_context_s) * plane->contexts_len;
	contexts = malloc_or_die("contexts", size);
	plane->contexts = contexts;
	plane->num_threads = num_threads;

	coordinate_plane_reset(plane, win_width, win_height, center,
			       resolution_x, resolution_y, pfunc_idx, seed);

	return plane;
}

void coordinate_plane_free(coordinate_plane_s *plane)
{
	if (plane) {
		free(plane->contexts);
#ifndef SKIP_THREADS
		if (plane->tpool) {
			basic_thread_pool_stop_and_free(&(plane->tpool));
		}
#endif
		free(plane->all_points);
		plane->all_points = NULL;
		plane->all_points_len = 0;

		free(plane->scratch);
		plane->scratch = NULL;
		plane->scratch_len = 0;

		free(plane->points_not_escaped);
		plane->points_not_escaped = NULL;
		plane->points_not_escaped_len = 0;
	}
	free(plane);
}

void coordinate_plane_resize(coordinate_plane_s *plane, uint32_t new_win_width,
			     uint32_t new_win_height, bool preserve_ratio)
{
	long double x_min = coordinate_plane_x_min(plane);
	long double x_max = coordinate_plane_x_max(plane);
	long double new_resolution_x = (x_max - x_min) / (1.0 * new_win_width);

	long double new_resolution_y = new_resolution_x;
	if (preserve_ratio) {
		long double y_min = coordinate_plane_y_min(plane);
		long double y_max = coordinate_plane_y_max(plane);
		new_resolution_y = (y_max - y_min) / (1.0 * new_win_height);
	}

	coordinate_plane_reset(plane, new_win_width, new_win_height,
			       plane->center, new_resolution_x,
			       new_resolution_y, plane->pfuncs_idx,
			       plane->seed);
}

static void
coordinate_plane_iterate_context_init(coordinate_plane_s *plane,
				      size_t steps, size_t offset,
				      size_t step_size)
{
	coordinate_plane_iterate_context_s *ctx = plane->contexts + offset;
	ctx->plane = plane;
	ctx->steps = steps;
	ctx->offset = offset;
	ctx->step_size = step_size;
	ctx->done = false;
	size_t scratch_offset = offset * (plane->scratch_len / step_size);
	ctx->not_escaped = plane->scratch + scratch_offset;
}

static void coordinate_plane_update_from_iterate_context(coordinate_plane_s
							 *plane, coordinate_plane_iterate_context_s
							 *ctx)
{
	plane->escaped += ctx->local_escaped;
	iterxy_s **start = plane->points_not_escaped + plane->not_escaped;
	size_t size = sizeof(iterxy_s *) * ctx->local_not_escaped;
	memcpy(start, ctx->not_escaped, size);
	plane->not_escaped += ctx->local_not_escaped;
	ctx->not_escaped_len = 0;
}

int
coordinate_plane_iterate_with_context(coordinate_plane_iterate_context_s *ctx)
{
	coordinate_plane_s *plane = ctx->plane;

	pfunc_f pfunc = pfuncs[plane->pfuncs_idx].pfunc;
	pfunc_escape_f pfunc_escape = pfuncs[plane->pfuncs_idx].pfunc_escape;

	ctx->local_escaped = 0;
	ctx->local_not_escaped = 0;
	for (size_t j = ctx->offset; j < plane->not_escaped;
	     j += ctx->step_size) {
		iterxy_s *p = plane->points_not_escaped[j];

		for (size_t i = 0; i < ctx->steps && !p->escaped; ++i) {
			if (pfunc_escape(p->z)) {
				p->escaped = plane->iteration_count + i + 1;
			} else {
				pfunc(p);
			}
		}

		if (p->escaped) {
			++(ctx->local_escaped);
		} else {
			ctx->not_escaped[ctx->local_not_escaped] = p;
			++(ctx->local_not_escaped);
		}
	}

	ctx->done = true;

	return 0;
}

static void coordinate_plane_iterate_single_threaded(coordinate_plane_s *plane,
						     uint32_t steps)
{
	size_t offset = 0;
	coordinate_plane_iterate_context_init(plane, steps, offset, 1);

	coordinate_plane_iterate_context_s *context = plane->contexts + offset;
	coordinate_plane_iterate_with_context(context);

	plane->not_escaped = 0;
	coordinate_plane_update_from_iterate_context(plane, context);
}

#ifndef SKIP_THREADS

static int coordinate_plane_iterate_inner(void *void_context)
{
	coordinate_plane_iterate_context_s *context = NULL;
	context = (coordinate_plane_iterate_context_s *)void_context;

	return coordinate_plane_iterate_with_context(context);
}

static void coordinate_plane_iterate_multi_threaded(coordinate_plane_s *plane,
						    uint32_t steps)
{
	size_t num_threads = plane->num_threads;
	if (num_threads < 2) {
		coordinate_plane_iterate_single_threaded(plane, steps);
		return;
	}

	basic_thread_pool_s *pool = plane->tpool;
	if (pool == NULL || basic_thread_pool_size(pool) < num_threads) {
		if (pool) {
			basic_thread_pool_stop_and_free(&(plane->tpool));
		}
		plane->tpool = basic_thread_pool_new(plane->num_threads);
		if (!plane->tpool) {
			die("plane->tpool is %p", plane->tpool);
		}
	}

	for (size_t i = 0; i < num_threads; ++i) {
		coordinate_plane_iterate_context_init(plane, steps, i,
						      num_threads);
		void *arg = plane->contexts + i;
		thrd_start_t func = coordinate_plane_iterate_inner;
		basic_thread_pool_add(plane->tpool, func, arg);
	}
	thrd_yield();
	basic_thread_pool_wait(plane->tpool);

	plane->not_escaped = 0;
	for (size_t i = 0; i < num_threads; ++i) {
		while (!plane->contexts[i].done) {
			thrd_yield();
		}
		coordinate_plane_iterate_context_s *ctx = plane->contexts + i;
		coordinate_plane_update_from_iterate_context(plane, ctx);
	}
}

#endif /* #ifndef SKIP_THREADS */

size_t coordinate_plane_iterate(coordinate_plane_s *plane, uint32_t steps)
{
	size_t old_escaped = plane->escaped;
	size_t previous_not_escaped = plane->not_escaped;
	size_t halt_after = coordinate_plane_halt_after(plane);

	if (halt_after) {
		uint64_t it_count = coordinate_plane_iteration_count(plane);
		uint64_t remaining = 0;
		if (it_count < halt_after) {
			remaining = halt_after - it_count;
		}
		if (steps > remaining) {
			steps = remaining;
		}
	}

	if (steps && plane->not_escaped) {
#ifndef SKIP_THREADS
		coordinate_plane_iterate_multi_threaded(plane, steps);
#else
		coordinate_plane_iterate_single_threaded(plane, steps);
#endif /* #ifndef SKIP_THREADS */

		plane->iteration_count += steps;

		if (previous_not_escaped != plane->not_escaped) {
			plane->unchanged = 0;
		} else {
			plane->unchanged += steps;
		}
	}

	assert(plane->escaped >= old_escaped);
	size_t newly_escaped = plane->escaped - old_escaped;
	return newly_escaped;
}

void coordinate_plane_next_function(coordinate_plane_s *plane)
{
	size_t old_pfuncs_idx = plane->pfuncs_idx;
	size_t new_pfuncs_idx = 1 + old_pfuncs_idx;
	if (new_pfuncs_idx >= pfuncs_len) {
		new_pfuncs_idx = 0;
	}

	ldxy_s center;
	ldxy_s seed;
	if (new_pfuncs_idx == pfuncs_julia_idx) {
		seed = plane->center;
		center = plane->seed;
	} else if (old_pfuncs_idx == pfuncs_julia_idx) {
		seed = plane->center;
		center = plane->seed;
	} else {
		center = plane->center;
		seed = plane->seed;
	}

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       center, plane->resolution_x, plane->resolution_y,
			       new_pfuncs_idx, seed);
}

void coordinate_plane_zoom_in(coordinate_plane_s *plane)
{
	long double ratio_x = 0.8;
	long double ratio_y = 0.8;

	long double new_resolution_x = plane->resolution_x * ratio_x;
	long double new_resolution_y = plane->resolution_y * ratio_y;

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       plane->center, new_resolution_x,
			       new_resolution_y, plane->pfuncs_idx,
			       plane->seed);
}

void coordinate_plane_zoom_out(coordinate_plane_s *plane)
{
	long double ratio_x = 1.25;
	long double ratio_y = 1.25;

	long double new_resolution_x = plane->resolution_x * ratio_x;
	long double new_resolution_y = plane->resolution_y * ratio_y;

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       plane->center, new_resolution_x,
			       new_resolution_y, plane->pfuncs_idx,
			       plane->seed);
}

void coordinate_plane_pan_left(coordinate_plane_s *plane)
{
	ldxy_s new_center;
	new_center.y = plane->center.y;

	long double x_min = coordinate_plane_x_min(plane);
	long double x_max = coordinate_plane_x_max(plane);
	long double x_span = x_max - x_min;
	new_center.x = (plane->center.x - (x_span / 8));

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       new_center, plane->resolution_x,
			       plane->resolution_y, plane->pfuncs_idx,
			       plane->seed);
}

void coordinate_plane_pan_right(coordinate_plane_s *plane)
{
	ldxy_s new_center;
	new_center.y = plane->center.y;

	long double x_min = coordinate_plane_x_min(plane);
	long double x_max = coordinate_plane_x_max(plane);
	long double x_span = x_max - x_min;
	new_center.x = (plane->center.x + (x_span / 8));

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       new_center, plane->resolution_x,
			       plane->resolution_y, plane->pfuncs_idx,
			       plane->seed);
}

void coordinate_plane_pan_up(coordinate_plane_s *plane)
{
	ldxy_s new_center;
	new_center.x = plane->center.x;

	long double y_min = coordinate_plane_y_min(plane);
	long double y_max = coordinate_plane_y_max(plane);
	long double y_span = y_max - y_min;
	new_center.y = (plane->center.y + (y_span / 8));

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       new_center, plane->resolution_x,
			       plane->resolution_y, plane->pfuncs_idx,
			       plane->seed);
}

void coordinate_plane_pan_down(coordinate_plane_s *plane)
{
	ldxy_s new_center;
	new_center.x = plane->center.x;

	long double y_min = coordinate_plane_y_min(plane);
	long double y_max = coordinate_plane_y_max(plane);
	long double y_span = y_max - y_min;
	new_center.y = (plane->center.y - (y_span / 8));

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       new_center, plane->resolution_x,
			       plane->resolution_y, plane->pfuncs_idx,
			       plane->seed);
}

void coordinate_plane_recenter(coordinate_plane_s *plane,
			       uint32_t x, uint32_t y)
{
	assert(x < plane->win_width);
	assert(y < plane->win_height);

	iterxy_s *p = plane->all_points + ((plane->win_width * y) + x);

	coordinate_plane_reset(plane, plane->win_width, plane->win_height, p->c,
			       plane->resolution_x, plane->resolution_y,
			       plane->pfuncs_idx, plane->seed);
}

uint32_t coordinate_plane_win_width(coordinate_plane_s *plane)
{
	return plane->win_width;
}

uint32_t coordinate_plane_win_height(coordinate_plane_s *plane)
{
	return plane->win_height;
}

void coordinate_plane_threads_more(coordinate_plane_s *plane)
{
	++(plane->num_threads);
	if (plane->num_threads > plane->contexts_len) {
		free(plane->contexts);
		plane->contexts = NULL;
		plane->contexts_len = plane->num_threads;
		size_t size =
		    sizeof(coordinate_plane_iterate_context_s) *
		    plane->contexts_len;
		coordinate_plane_iterate_context_s *contexts;
		contexts = malloc_or_die("contexts", size);
		plane->contexts = contexts;
	}
}

void coordinate_plane_threads_less(coordinate_plane_s *plane)
{
	if (plane->num_threads > 1) {
		--(plane->num_threads);
	}
}

const char *coordinate_plane_program(coordinate_plane_s *plane)
{
	return plane->argv0;
}

const char *coordinate_plane_function_name(coordinate_plane_s *plane)
{
	return pfuncs[plane->pfuncs_idx].name;
}

size_t coordinate_plane_function_index(coordinate_plane_s *plane)
{
	return plane->pfuncs_idx;
}

void coordinate_plane_center(coordinate_plane_s *plane, ldxy_s *out)
{
	out->x = plane->center.x;
	out->y = plane->center.y;
}

void coordinate_plane_seed(coordinate_plane_s *plane, ldxy_s *out)
{
	out->x = plane->seed.x;
	out->y = plane->seed.y;
}

long double coordinate_plane_resolution_x(coordinate_plane_s *plane)
{
	return plane->resolution_x;
}

long double coordinate_plane_resolution_y(coordinate_plane_s *plane)
{
	return plane->resolution_y;
}

uint64_t coordinate_plane_halt_after(coordinate_plane_s *plane)
{
	return plane->halt_after;
}

uint32_t coordinate_plane_skip_rounds(coordinate_plane_s *plane)
{
	return plane->skip_rounds;
}

uint64_t coordinate_plane_escaped(coordinate_plane_s *plane, uint32_t x,
				  uint32_t y)
{
	size_t i = (y * plane->win_width) + x;
	iterxy_s *p = plane->all_points + i;
	return p->escaped;
}

uint64_t coordinate_plane_iteration_count(coordinate_plane_s *plane)
{
	return plane->iteration_count;
}

size_t coordinate_plane_escaped_count(coordinate_plane_s *plane)
{
	return plane->escaped;
}

size_t coordinate_plane_not_escaped_count(coordinate_plane_s *plane)
{
	return plane->not_escaped + plane->trapped;
}

size_t coordinate_plane_trapped_count(coordinate_plane_s *plane)
{
	return plane->trapped;
}

size_t coordinate_plane_unchanged(coordinate_plane_s *plane)
{
	return plane->unchanged;
}

size_t coordinate_plane_num_threads(coordinate_plane_s *plane)
{
	return plane->num_threads;
}
