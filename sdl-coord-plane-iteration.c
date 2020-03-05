/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* sdl-coord-plane-iteration.c: playing with mandlebrot and such */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */
/*
   cc -g -O2 `sdl2-config --cflags` \
      ./sdl-coord-plane-iteration.c \
      -o ./sdl-coord-plane-iteration \
      `sdl2-config --libs` -lm -lpthread &&
   ./sdl-coord-plane-iteration
*/

#define SDL_COORD_PLANE_ITERATION_VERSION "0.1.0"

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <SDL.h>

#ifndef SKIP_THREADS
#include <threads.h>
#include <unistd.h>
#endif

#ifndef Make_valgrind_happy
#define Make_valgrind_happy 0
#endif

#define die(format, ...) \
	do { \
		fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
		/* fprintf(stderr, format __VA_OPT__(,) __VA_ARGS__); */ \
		/* fprintf(stderr, format, ##_VA_ARGS__); */ \
		fprintf(stderr, format, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
		exit(EXIT_FAILURE); \
	} while (0)

#define alloc_or_exit(ptr, size) \
	do { \
		size_t _alloc_or_exit_size_t = (size_t)(size); \
		ptr = calloc(1, _alloc_or_exit_size_t); \
		if (!ptr) { \
			die("could not allocate %zu bytes for %s?", \
					_alloc_or_exit_size_t, #ptr); \
		} \
	} while (0)

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

typedef struct iterxy {
	ldxy_s seed;

	/* coordinate location */
	ldxy_s c;

	/* calculated next location */
	ldxy_s z;

	uint32_t escaped;
} iterxy_s;

static void iterxy_init_zero(iterxy_s *p, ldxy_s xy, ldxy_s seed)
{
	p->seed = seed;
	p->c.x = xy.x;
	p->c.y = xy.y;
	p->z.x = 0.0;
	p->z.y = 0.0;
	p->escaped = 0;
}

static void iterxy_init_xy(iterxy_s *p, ldxy_s xy, ldxy_s seed)
{
	p->seed = seed;
	p->c.x = xy.x;
	p->c.y = xy.y;
	p->z.x = xy.x;
	p->z.y = xy.y;
	p->escaped = 0;
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

/* Z[n+1] = (Z[n])^2 + Orig */
static void mandlebrot(iterxy_s *p)
{
	/* first, square the complex */
	ldxy_s result;
	square_complex(&result, p->z);

	/* then add the original C to the Z[n]^2 result */
	p->z.x = result.x + p->c.x;
	p->z.y = result.y + p->c.y;
}

static void julia(iterxy_s *p)
{
	/* first, square the complex */
	ldxy_s result;
	square_complex(&result, p->z);

	/* then add the seed C to the Z[n]^2 result */
	p->z.x = result.x + p->seed.x;
	p->z.y = result.y + p->seed.y;
}

#ifdef INCLUDE_ALL_FUNCTIONS
static void ordinary_square(iterxy_s *p)
{
	p->z.y = p->z.y * p->z.y;
	p->z.x = p->z.x * p->z.x;
}

/* Z[n+1] = collapse_to_y2_to_y((Z[n])^2) + Orig */
static void square_binomial_collapse_y2_add_orig(iterxy_s *p)
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
static void square_binomial_ignore_y2_add_orig(iterxy_s *p)
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

static void not_a_circle(iterxy_s *p)
{
	long double xx = p->z.x * p->z.x;
	long double yy = p->z.y * p->z.y;

	p->z.y = yy + (0.5 * p->z.x);
	p->z.x = xx + (0.5 * p->z.y);
}
#endif /* INCLUDE_ALL_FUNCTIONS */

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
named_pfunc_s pfuncs[] = {
	{ iterxy_init_zero, xy_radius_greater_than_2, mandlebrot,
	 "mandlebrot" },
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

typedef struct coordinate_plane {
	const char *argv0;

	uint32_t win_width;
	uint32_t win_height;

	ldxy_s center;
	long double resolution;

	uint32_t iteration_count;
	size_t escaped;
	size_t not_escaped;
	uint32_t skip_rounds;

	uint32_t num_threads;

	size_t pfuncs_idx;
	ldxy_s seed;

	iterxy_s *points;
	size_t len;
} coordinate_plane_s;

static long double coordinate_plane_x_min(coordinate_plane_s *plane)
{
	return plane->center.x - (plane->resolution * (plane->win_width / 2));
}

static long double coordinate_plane_y_min(coordinate_plane_s *plane)
{
	return plane->center.y - (plane->resolution * (plane->win_height / 2));
}

static long double coordinate_plane_x_max(coordinate_plane_s *plane)
{
	return plane->center.x + (plane->resolution * (plane->win_width / 2));
}

static long double coordinate_plane_y_max(coordinate_plane_s *plane)
{
	return plane->center.y + (plane->resolution * (plane->win_height / 2));
}

coordinate_plane_s *coordinate_plane_reset(coordinate_plane_s *plane,
					   uint32_t win_width,
					   uint32_t win_height,
					   ldxy_s center,
					   long double resolution,
					   size_t pfuncs_idx, ldxy_s seed)
{
	plane->win_width = win_width;
	plane->win_height = win_height;
	plane->center = center;
	plane->resolution = resolution;
	if (!(plane->resolution > 0.0)) {
		die("invalid resolution %.*Lg", DECIMAL_DIG, resolution);
	}
	plane->iteration_count = 0;
	plane->escaped = 0;
	plane->not_escaped = (plane->win_width * plane->win_height);
	plane->pfuncs_idx = pfuncs_idx;
	/* cache the seed on the plane for reset */
	plane->seed = seed;

	size_t needed = win_width * win_height;
	if (plane->points && (plane->len < needed)) {
		free(plane->points);
		plane->points = NULL;
	}
	if (!plane->points) {
		plane->len = needed;
		size_t size = plane->len * sizeof(iterxy_s);
		alloc_or_exit(plane->points, size);
	}

	pfunc_init_f pfunc_init = pfuncs[plane->pfuncs_idx].pfunc_init;
	long double x_min = coordinate_plane_x_min(plane);
	long double y_max = coordinate_plane_y_max(plane);
	for (size_t py = 0; py < plane->win_height; ++py) {
		for (size_t px = 0; px < plane->win_width; ++px) {
			size_t i = (py * plane->win_width) + px;

			iterxy_s *p = plane->points + i;

			/* location on the co-ordinate plane */
			ldxy_s xy;
			xy.y = y_max - (py * plane->resolution);
			if (fabsl(xy.y) < (plane->resolution / 2)) {
				/* near enought to zero to call it zero */
				xy.y = 0.0;
			}

			xy.x = x_min + px * plane->resolution;
			if (fabsl(xy.x) < (plane->resolution / 2)) {
				/* near enought to zero to call it zero */
				xy.x = 0.0;
			}

			pfunc_init(p, xy, seed);
		}
	}
	return plane;
}

coordinate_plane_s *coordinate_plane_new(const char *program_name,
					 uint32_t win_width,
					 uint32_t win_height,
					 ldxy_s center,
					 long double resolution,
					 size_t pfunc_idx,
					 ldxy_s seed,
					 uint32_t skip_rounds,
					 uint32_t num_threads)
{
	coordinate_plane_s *plane = NULL;
	size_t size = sizeof(coordinate_plane_s);

	alloc_or_exit(plane, size);

	plane->argv0 = program_name;
	plane->num_threads = num_threads;
	plane->skip_rounds = skip_rounds;

	coordinate_plane_reset(plane, win_width, win_height, center,
			       resolution, pfunc_idx, seed);

	return plane;
}

void coordinate_plane_free(coordinate_plane_s *plane)
{
	if (plane) {
		free(plane->points);
	}
	free(plane);
}

void coordinate_plane_resize(coordinate_plane_s *plane, uint32_t new_win_width,
			     uint32_t new_win_height)
{
	long double x_min = coordinate_plane_x_min(plane);
	long double x_max = coordinate_plane_x_max(plane);
	long double new_resolution = (x_max - x_min) / (1.0 * new_win_width);
	coordinate_plane_reset(plane, new_win_width, new_win_height,
			       plane->center, new_resolution, plane->pfuncs_idx,
			       plane->seed);
}

typedef struct coordinate_plane_iterate_context {
	coordinate_plane_s *plane;
	size_t steps;
	size_t offset;
	size_t step_size;
} coordinate_plane_iterate_context_s;

static int coordinate_plane_iterate_context(coordinate_plane_iterate_context_s
					    *ctx)
{
	coordinate_plane_s *plane = ctx->plane;

	pfunc_f pfunc = pfuncs[plane->pfuncs_idx].pfunc;
	pfunc_escape_f pfunc_escape = pfuncs[plane->pfuncs_idx].pfunc_escape;

	int local_escaped = 0;
	for (size_t j = ctx->offset; j < plane->len; j += ctx->step_size) {
		iterxy_s *p = plane->points + j;

		for (size_t i = 0; i < ctx->steps && !p->escaped; ++i) {

			if (pfunc_escape(p->z)) {
				p->escaped = plane->iteration_count + i + 1;
				continue;
			}

			pfunc(p);
		}

		if (p->escaped) {
			++local_escaped;
		}
	}

	return local_escaped;
}

static void coordinate_plane_iterate_single_threaded(coordinate_plane_s *plane,
						     uint32_t steps)
{
	coordinate_plane_iterate_context_s context;
	context.plane = plane;
	context.steps = steps;
	context.offset = 0;
	context.step_size = 1;

	int result = coordinate_plane_iterate_context(&context);

	size_t local_escaped = (size_t)result;
	plane->not_escaped -= local_escaped;
	plane->escaped += local_escaped;
}

#ifndef SKIP_THREADS

static int coordinate_plane_iterate_inner(void *void_context)
{
	coordinate_plane_iterate_context_s *context = NULL;
	context = (coordinate_plane_iterate_context_s *)void_context;

	return coordinate_plane_iterate_context(context);
}

// we don't really need this boiler-plate to be this verbose
#define thrd_create_or_die(our_id, thread_id, thread_func, context) do { \
	int _thrd_err = thrd_create(thread_id, thread_func, context); \
	switch (_thrd_err) { \
	case thrd_success: \
		break; \
	case thrd_nomem: \
		die("thrd_create for %zu returned thrd_nomem\n", our_id); \
		break; \
	case thrd_error: \
		die("thrd_create for %zu returned thrd_error\n", our_id); \
		break; \
	case thrd_timedout: \
		die("thrd_create for %zu returned thrd_timedout?\n", our_id); \
		break; \
	case thrd_busy: \
		die("thrd_create for %zu returned thrd_busy?\n", our_id); \
		break; \
	default: \
		die("thrd_create for %zu returned %d?\n", our_id, _thrd_err); \
		break; \
	} \
} while (0)

static void coordinate_plane_iterate_multi_threaded(coordinate_plane_s *plane,
						    uint32_t steps)
{
	size_t num_threads = plane->num_threads;
	if (num_threads < 2) {
		coordinate_plane_iterate_single_threaded(plane, steps);
		return;
	}

	thrd_t *thread_ids = NULL;
	size_t size = sizeof(thrd_t) * num_threads;
	alloc_or_exit(thread_ids, size);

	coordinate_plane_iterate_context_s *contexts;
	size = sizeof(coordinate_plane_iterate_context_s) * num_threads;
	alloc_or_exit(contexts, size);

	for (size_t i = 0; i < num_threads; ++i) {
		contexts[i].plane = plane;
		contexts[i].steps = steps;
		contexts[i].offset = i;
		contexts[i].step_size = num_threads;
	}

	for (size_t i = 0; i < num_threads; ++i) {
		void *context = &(contexts[i]);
		thrd_start_t thread_func = &coordinate_plane_iterate_inner;
		thrd_t *thread_id = &(thread_ids[i]);

		thrd_create_or_die(i, thread_id, thread_func, context);
	}

	for (size_t i = 0; i < num_threads; ++i) {
		int result = 0;
		thrd_join(thread_ids[i], &result);
		size_t local_escaped = (size_t)result;
		plane->not_escaped -= local_escaped;
		plane->escaped += local_escaped;
	}

	free(thread_ids);
	free(contexts);
}

#endif /* #ifndef SKIP_THREADS */

size_t coordinate_plane_iterate(coordinate_plane_s *plane, uint32_t steps)
{
	size_t old_escaped = plane->escaped;

	plane->escaped = 0;
	plane->not_escaped = plane->win_height * plane->win_width;

#ifndef SKIP_THREADS
	coordinate_plane_iterate_multi_threaded(plane, steps);
#else
	coordinate_plane_iterate_single_threaded(plane, steps);
#endif /* #ifndef SKIP_THREADS */

	plane->iteration_count += steps;

	return plane->escaped - old_escaped;
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
			       center, plane->resolution, new_pfuncs_idx, seed);
}

void coordinate_plane_zoom_in(coordinate_plane_s *plane)
{
	long double new_resolution = plane->resolution * 0.8;

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       plane->center, new_resolution,
			       plane->pfuncs_idx, plane->seed);
}

void coordinate_plane_zoom_out(coordinate_plane_s *plane)
{
	long double new_resolution = plane->resolution * 1.25;

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       plane->center, new_resolution,
			       plane->pfuncs_idx, plane->seed);
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
			       new_center, plane->resolution,
			       plane->pfuncs_idx, plane->seed);
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
			       new_center, plane->resolution,
			       plane->pfuncs_idx, plane->seed);
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
			       new_center, plane->resolution,
			       plane->pfuncs_idx, plane->seed);
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
			       new_center, plane->resolution,
			       plane->pfuncs_idx, plane->seed);
}

void coordinate_plane_recenter(coordinate_plane_s *plane,
			       uint32_t x, uint32_t y)
{
	assert(x < plane->win_width);
	assert(y < plane->win_height);

	iterxy_s *p = plane->points + ((plane->win_width * y) + x);

	coordinate_plane_reset(plane, plane->win_width, plane->win_height,
			       p->c, plane->resolution, plane->pfuncs_idx,
			       plane->seed);
}

typedef struct rgb24 {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} rgb24_s;

// values are from 0.0 to 1.0
typedef struct rgb {
	double red;
	double green;
	double blue;
} rgb_s;

// hue is 0.0 to 360.0
// saturation and value are 0.0 to 0.1
typedef struct hsv {
	double hue;
	double sat;
	double val;
} hsv_s;

#ifndef NDEBUG
static bool invalid_hsv_s(hsv_s hsv)
{
	if (!(hsv.hue >= 0.0 && hsv.hue <= 360.0)) {
		return true;
	}
	if (!(hsv.sat >= 0 && hsv.sat <= 1.0)) {
		return true;
	}
	if (!(hsv.val >= 0 && hsv.val <= 1.0)) {
		return true;
	}
	return false;
}
#endif

void rgb24_from_rgb(rgb24_s *out, rgb_s in)
{
	out->red = 255 * in.red;
	out->green = 255 * in.green;
	out->blue = 255 * in.blue;
}

uint32_t rgb24_to_uint32(rgb24_s rgb)
{
	uint32_t urgb = ((rgb.red << 16) + (rgb.green << 8) + rgb.blue);
	return urgb;
}

// https://dystopiancode.blogspot.com/2012/06/hsv-rgb-conversion-algorithms-in-c.html
// https://en.wikipedia.org/wiki/HSL_and_HSV
bool rgb_from_hsv(rgb_s *rgb, hsv_s hsv)
{
#ifndef NDEBUG
	if (!rgb || invalid_hsv_s(hsv)) {
		return false;
	}
#endif

	double hue = hsv.hue == 360.0 ? 0.0 : hsv.hue;
	double chroma = hsv.val * hsv.sat;
	double offset = chroma * (1.0 - fabs(fmod(hue / 60.0, 2) - 1.0));
	double smallm = hsv.val - chroma;

	if (hue >= 0.0 && hue < 60.0) {
		rgb->red = chroma + smallm;
		rgb->green = offset + smallm;
		rgb->blue = smallm;
	} else if (hue >= 60.0 && hue < 120.0) {
		rgb->red = offset + smallm;
		rgb->green = chroma + smallm;
		rgb->blue = smallm;
	} else if (hue >= 120.0 && hue < 180.0) {
		rgb->red = smallm;
		rgb->green = chroma + smallm;
		rgb->blue = offset + smallm;
	} else if (hue >= 180.0 && hue < 240.0) {
		rgb->red = smallm;
		rgb->green = offset + smallm;
		rgb->blue = chroma + smallm;
	} else if (hue >= 240.0 && hue < 300.0) {
		rgb->red = offset + smallm;
		rgb->green = smallm;
		rgb->blue = chroma + smallm;
	} else if (hue >= 300.0 && hue < 360.0) {
		rgb->red = chroma + smallm;
		rgb->green = smallm;
		rgb->blue = offset + smallm;
	} else {
		rgb->red = smallm;
		rgb->green = smallm;
		rgb->blue = smallm;
	}

	return true;
}

static void long_tail_gradiant(rgb24_s *result, uint32_t distance)
{
	double log_divisor = 8;
	// factor should be 0.0 t/m 1.0
	double factor = fmod(log2(distance) / log_divisor, 1.0);
	double hue = 360.0 * factor;

#if DEBUG
	if (distance <= 10 || (distance % 100 == 0)) {
		fprintf(stdout, "\ni: %" PRIu32 ": hue: %g\n", distance, hue);
	}
#endif

	double saturation = 1.0;
	double value = 1.0;
	hsv_s hsv = { hue, saturation, value };
	rgb_s rgb = { 0x00, 0x00, 0x00 };
	rgb_from_hsv(&rgb, hsv);
	rgb24_from_rgb(result, rgb);
}

typedef struct keyboard_key {
	uint32_t is_down:1;
	uint32_t was_down:1;
} keyboard_key_s;

typedef struct human_input {
	keyboard_key_s up;
	keyboard_key_s w;

	keyboard_key_s left;
	keyboard_key_s a;

	keyboard_key_s down;
	keyboard_key_s s;

	keyboard_key_s right;
	keyboard_key_s d;

	keyboard_key_s page_up;
	keyboard_key_s z;

	keyboard_key_s page_down;
	keyboard_key_s x;

	keyboard_key_s m;
	keyboard_key_s n;

	keyboard_key_s q;
	keyboard_key_s space;
	keyboard_key_s esc;

	uint8_t click;
	uint32_t click_x;
	uint32_t click_y;

	int wheel_zoom;
} human_input_s;

void human_input_init(human_input_s *input)
{
	input->up.is_down = 0;
	input->up.was_down = 0;

	input->w.is_down = 0;
	input->w.was_down = 0;

	input->left.is_down = 0;
	input->left.was_down = 0;

	input->a.is_down = 0;
	input->a.was_down = 0;

	input->down.is_down = 0;
	input->down.was_down = 0;

	input->s.is_down = 0;
	input->s.was_down = 0;

	input->right.is_down = 0;
	input->right.was_down = 0;

	input->d.is_down = 0;
	input->d.was_down = 0;

	input->page_down.is_down = 0;
	input->page_down.was_down = 0;

	input->z.is_down = 0;
	input->z.was_down = 0;

	input->page_up.is_down = 0;
	input->page_up.was_down = 0;

	input->x.is_down = 0;
	input->x.was_down = 0;

	input->m.is_down = 0;
	input->m.was_down = 0;

	input->n.is_down = 0;
	input->n.was_down = 0;

	input->q.is_down = 0;
	input->q.was_down = 0;

	input->space.is_down = 0;
	input->space.was_down = 0;

	input->esc.is_down = 0;
	input->esc.was_down = 0;

	input->click = 0;
	input->click_x = 0;
	input->click_y = 0;

	input->wheel_zoom = 0;
}

enum coordinate_plane_change {
	coordinate_plane_change_shutdown = -1,
	coordinate_plane_change_no = 0,
	coordinate_plane_change_yes = 1,
};

enum coordinate_plane_change human_input_process(human_input_s *input,
						 coordinate_plane_s *plane)
{
	if ((input->esc.is_down) || (input->q.is_down)) {
		return coordinate_plane_change_shutdown;
	}

	if (input->space.is_down) {
		coordinate_plane_next_function(plane);
		return coordinate_plane_change_yes;
	}
#ifndef SKIP_THREADS
	if (input->m.is_down && !input->m.was_down) {
		++plane->num_threads;
		return coordinate_plane_change_no;
	}
	if (input->n.is_down && !input->n.was_down) {
		if (plane->num_threads > 1) {
			--plane->num_threads;
		}
		return coordinate_plane_change_no;
	}
#endif

	if ((input->w.is_down && !input->w.was_down) ||
	    (input->up.is_down && !input->up.was_down)) {
		coordinate_plane_pan_up(plane);
		return coordinate_plane_change_yes;
	}
	if ((input->s.is_down && !input->s.was_down) ||
	    (input->down.is_down && !input->down.was_down)) {
		coordinate_plane_pan_down(plane);
		return coordinate_plane_change_yes;
	}

	if ((input->a.is_down && !input->a.was_down) ||
	    (input->left.is_down && !input->left.was_down)) {
		coordinate_plane_pan_left(plane);
		return coordinate_plane_change_yes;
	}
	if ((input->d.is_down && !input->d.was_down) ||
	    (input->right.is_down && !input->right.was_down)) {
		coordinate_plane_pan_right(plane);
		return coordinate_plane_change_yes;
	}
	if ((input->x.is_down && !input->x.was_down) ||
	    (input->page_up.is_down && !input->page_up.was_down) ||
	    (input->wheel_zoom < 0)) {
		coordinate_plane_zoom_out(plane);
		return coordinate_plane_change_yes;
	}
	if ((input->z.is_down && !input->z.was_down) ||
	    (input->page_down.is_down && !input->page_down.was_down) ||
	    (input->wheel_zoom > 0)) {
		coordinate_plane_zoom_in(plane);
		return coordinate_plane_change_yes;
	}

	if (input->click) {
		coordinate_plane_recenter(plane, input->click_x,
					  input->click_y);
		return coordinate_plane_change_yes;
	}

	return coordinate_plane_change_no;
}

void print_directions(coordinate_plane_s *plane, FILE *out)
{
	const char *title = pfuncs[plane->pfuncs_idx].name;
	fprintf(out, "\n\n%s\n", title);
	fprintf(out, "%s --function=%zu", plane->argv0, plane->pfuncs_idx);
	if (plane->pfuncs_idx == pfuncs_julia_idx) {
		fprintf(out, " --seed_x=%.*Lg --seed_y=%.*Lg", DECIMAL_DIG,
			plane->seed.x, DECIMAL_DIG, plane->seed.y);
	}
	if (plane->skip_rounds) {
		fprintf(out, " --skip_rounds=%" PRIu32, plane->skip_rounds);
	}
	fprintf(out, " --center_x=%.*Lg --center_y=%.*Lg", DECIMAL_DIG,
		plane->center.x, DECIMAL_DIG, plane->center.y);
	long double x_min = coordinate_plane_x_min(plane);
	long double x_max = coordinate_plane_x_max(plane);
	fprintf(out, " --from=%.*Lg --to=%.*Lg", DECIMAL_DIG, x_min,
		DECIMAL_DIG, x_max);
	fprintf(out, " --width=%" PRIu32, plane->win_width);
	fprintf(out, " --height=%" PRIu32, plane->win_height);
	fprintf(out, "\n");
	long double y_min = coordinate_plane_y_min(plane);
	long double y_max = coordinate_plane_y_max(plane);
	fprintf(out,
		"(y-axis co-ordinates range from: %.*Lg to: %.*Lg)\n",
		DECIMAL_DIG, y_min, DECIMAL_DIG, y_max);
	fprintf(out, "use arrows or 'wasd' keys to pan\n");
	fprintf(out,
		"use page_down/page_up or 'z' and 'x' keys to zoom in/out\n");
	fprintf(out, "space will cycle through available functions\n");
	fprintf(out, "click to recenter the image\n");
	fprintf(out, "escape or 'q' to quit\n");
	fflush(out);
}

typedef struct pixel_buffer {
	uint32_t width;
	uint32_t height;
	uint8_t bytes_per_pixel;
	size_t pixels_len;
	/* pitch is bytes in a row of pixel data, including padding */
	uint32_t pitch;
	uint32_t *pixels;
	rgb24_s *palette;
	size_t palette_len;
} pixel_buffer_s;

void pixel_buffer_update(coordinate_plane_s *plane, pixel_buffer_s *buf)
{
	if (plane->win_width != buf->width) {
		die("plane->win_width:%" PRIu32 " != buf->width: %" PRIu32,
		    plane->win_width, buf->width);
	}
	if (plane->win_height != buf->height) {
		die("plane->win_height:%" PRIu32 " != buf->height: %" PRIu32,
		    plane->win_height, buf->height);
	}

	for (uint32_t y = 0; y < plane->win_height; y++) {
		for (uint32_t x = 0; x < plane->win_width; x++) {
			size_t i = (y * plane->win_width) + x;
			iterxy_s *p = plane->points + i;
			size_t palette_idx = p->escaped % buf->palette_len;
			rgb24_s color = buf->palette[palette_idx];
			uint32_t foreground = rgb24_to_uint32(color);
			*(buf->pixels + (y * buf->width) + x) = foreground;
		}
	}
}

void *pixel_buffer_resize(pixel_buffer_s *buf, int height, int width)
{
	if (buf->pixels) {
		free(buf->pixels);
	}
	buf->width = width;
	buf->height = height;
	buf->pixels_len = buf->height * buf->width;
	buf->pitch = buf->width * buf->bytes_per_pixel;
	size_t size = buf->pixels_len * buf->bytes_per_pixel;

	alloc_or_exit(buf->pixels, size);

	return buf->pixels;
}

static rgb24_s *grow_palette(rgb24_s *palette, size_t *len,
			     size_t prefix_black, size_t amount)
{
	size_t new_len = (amount + (*len));
	size_t size = sizeof(rgb24_s) * new_len;
	rgb24_s *grow = realloc(palette, size);
	if (!grow) {
		fprintf(stderr,
			"could not allocate %zu bytes for palette[%zu]?",
			size, new_len);
		return palette;
	}
	palette = grow;
	size_t keep = *len;
	*len = new_len;
	for (size_t i = keep; i < prefix_black; ++i) {
		palette[i].red = 0;
		palette[i].green = 0;
		palette[i].blue = 0;
	}
	keep = keep >= prefix_black ? keep : prefix_black;
	for (size_t i = keep; i < new_len; ++i) {
		long_tail_gradiant(palette + i, i);
	}
	return palette;
}

pixel_buffer_s *pixel_buffer_new(uint32_t window_x, uint32_t window_y,
				 size_t palette_len, size_t skip_rounds)
{
	pixel_buffer_s *buf = NULL;
	size_t size = sizeof(pixel_buffer_s);
	alloc_or_exit(buf, size);

	buf->bytes_per_pixel = sizeof(uint32_t);
	pixel_buffer_resize(buf, window_y, window_x);

	buf->palette = NULL;
	buf->palette_len = 0;

	buf->palette =
	    grow_palette(buf->palette, &buf->palette_len, skip_rounds,
			 palette_len);
	if (!buf->palette) {
		size_t size = sizeof(rgb24_s) * palette_len;
		die("palette == NULL? (size %zu)", size);
	}

	return buf;
}

void pixel_buffer_free(pixel_buffer_s *buf)
{
	if (!buf) {
		return;
	}
	free(buf->palette);
	free(buf->pixels);
	free(buf);
}

typedef struct coord_options {
	int win_width;
	int win_height;
	long double x_min;
	long double x_max;
	long double center_x;
	long double center_y;
	long double seed_x;
	long double seed_y;
	int function;
	int threads;
	int skip_rounds;
	int version;
	int help;
} coord_options_s;

static void coord_options_init(coord_options_s *options)
{
	options->win_width = -1;
	options->win_height = -1;
	options->x_min = NAN;
	options->x_max = NAN;
	options->center_x = NAN;
	options->center_y = NAN;
	options->function = -1;
	options->seed_x = NAN;
	options->seed_y = NAN;
	options->threads = -1;
	options->skip_rounds = -1;
	options->version = 0;
	options->help = 0;
}

static void coord_options_rationalize(coord_options_s *options)
{
	if (options->help != 1) {
		options->help = 0;
	}
	if (options->version != 1) {
		options->version = 0;
	}
	if (options->win_width < 1) {
		options->win_width = 800;
	}
	if (options->win_height < 1) {
		options->win_height = ((options->win_width * 3) / 4);
	}
	if (!isfinite(options->x_min)) {
		options->x_min = -2.5;
	}
	if (!isfinite(options->x_max)) {
		options->x_max = options->x_min + 4.0;
	}
	if (!isfinite(options->center_x)) {
		options->center_x = -0.5;
	}
	if (!isfinite(options->center_y)) {
		options->center_y = 0.0;
	}
	if (options->function < 0
	    || (((size_t)options->function) >= pfuncs_len)) {
		options->function = pfuncs_mandlebrot_idx;
	}
	if (!isfinite(options->seed_x)) {
		options->seed_x = -1.25643;
	}
	if (!isfinite(options->seed_y)) {
		options->seed_y = -0.381086;
	}
	if (options->skip_rounds < 0) {
		options->skip_rounds = 0;
	}
#ifndef SKIP_THREADS
	if (options->threads < 1) {
		options->threads = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
	}
#endif
}

static int coord_options_parse_argv(coord_options_s *options,
				    int argc, char **argv, FILE *err)
{
	int opt_char;
	int option_index;

	/* yes, optstirng is horrible */
	const char *optstring = "HVw::h::x::y::f::t::j::r::i::c::s::";

	struct option long_options[] = {
		{ "help", no_argument, 0, 'H' },
		{ "version", no_argument, 0, 'V' },
		{ "width", optional_argument, 0, 'w' },
		{ "height", optional_argument, 0, 'h' },
		{ "center_x", optional_argument, 0, 'x' },
		{ "center_y", optional_argument, 0, 'y' },
		{ "from", optional_argument, 0, 'f' },
		{ "to", optional_argument, 0, 't' },
		{ "function", optional_argument, 0, 'j' },
		{ "seed_x", optional_argument, 0, 'r' },
		{ "seed_y", optional_argument, 0, 'i' },
		{ "threads", optional_argument, 0, 'c' },
		{ "skip_rounds", optional_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	while (1) {
		option_index = 0;

		opt_char =
		    getopt_long(argc, argv, optstring, long_options,
				&option_index);

		/* Detect the end of the options. */
		if (opt_char == -1) {
			break;
		}

		switch (opt_char) {
		case 0:
			break;
		case 'H':	/* --help | -H */
			options->help = 1;
			break;
		case 'V':	/* --version | -V */
			options->version = 1;
			break;
		case 'w':	/* --width | -w */
			options->win_width = atoi(optarg);
			break;
		case 'h':	/* --height | -h */
			options->win_height = atoi(optarg);
			break;
		case 'f':	/* --from | -f */
			options->x_min = strtold(optarg, NULL);
			break;
		case 't':	/* --to | -t */
			options->x_max = strtold(optarg, NULL);
			break;
		case 'x':	/* --center_x | -x */
			options->center_x = strtold(optarg, NULL);
			break;
		case 'y':	/* --center_y | -y */
			options->center_y = strtold(optarg, NULL);
			break;
		case 'j':	/* --function | -j */
			options->function = atoi(optarg);
			break;
		case 'r':	/* --seed_x | -r */
			options->center_x = strtold(optarg, NULL);
			break;
		case 'i':	/* --seed_y | -i */
			options->center_y = strtold(optarg, NULL);
			break;
		case 'c':	/* --threads | -c */
			options->threads = atoi(optarg);
			break;
		case 's':	/* --skip_rounds | -s */
			options->skip_rounds = atoi(optarg);
			break;
		default:
			options->help = 1;
			fprintf(err, "unrecognized option: '%c'\n", opt_char);
			fflush(err);
			break;
		}
	}
	return option_index;
}

static void print_help(FILE *out, const char *argv0, const char *version)
{
	if (!out) {
		return;
	}
	fprintf(out, "%s version %s\n", argv0, version);
	fprintf(out, "OPTIONS:\n");
	fprintf(out, "\t-w --width=n       Witdth of the window in pixels\n");
	fprintf(out, "\t-h --height=n      Height of the window in pixels\n");
	fprintf(out, "\t-x --center_x=f    Center of the x-axis, '0.0'\n");
	fprintf(out, "\t-y --center_y=f    Center of the y-axis, '0.0'\n");
	fprintf(out, "\t-f --from=f        Left of the x-axis\n");
	fprintf(out, "\t                           default is '-2.5'\n");
	fprintf(out, "\t-t --to=f          Right of the x-axis\n");
	fprintf(out, "\t                           default is '1.5'\n");
	fprintf(out, "\t-j --function=n    Function number\n");
	fprintf(out, "\t                           0 for Mandlebrot\n");
	fprintf(out, "\t                           1 for Julia\n");
	fprintf(out, "\t-r --seed_x=f      Real (x) part of the Julia seed\n");
	fprintf(out, "\t-i --seed_y=f      Imaginary (y) part of the seed\n");
#ifndef SKIP_THREADS
	fprintf(out, "\t-c --threads=n     Number of threads/cores to use\n");
#endif
	fprintf(out, "\t-s --skip_rounds=n Number of iterations left black\n");
	fprintf(out, "\t-v --version       Print version and exit\n");
	fprintf(out, "\t-h --help          This message and exit\n");
}

coordinate_plane_s *coordinate_plane_new_from_args(int argc, char **argv)
{
	coord_options_s options;
	coord_options_init(&options);
	coord_options_parse_argv(&options, argc, argv, stdout);
	coord_options_rationalize(&options);

	if (options.help) {
		print_help(stdout, argv[0], SDL_COORD_PLANE_ITERATION_VERSION);
		exit(EXIT_SUCCESS);
	}

	if (options.version) {
		fprintf(stdout, "%s\n", SDL_COORD_PLANE_ITERATION_VERSION);
		exit(EXIT_SUCCESS);
	}

	ldxy_s seed = { options.seed_x, options.seed_y };
	ldxy_s center = { options.center_x, options.center_y };
	long double resolution =
	    (options.x_max - options.x_min) / (1.0 * options.win_width);

	coordinate_plane_s *plane =
	    coordinate_plane_new(argv[0], options.win_width, options.win_height,
				 center, resolution, options.function, seed,
				 options.skip_rounds, options.threads);

	return plane;
}

uint64_t time_in_usec(void)
{
	struct timeval tv = { 0, 0 };
	struct timezone *tz = NULL;
	gettimeofday(&tv, tz);
	uint64_t usec_per_sec = (1000 * 1000);
	uint64_t time_in_micros = (usec_per_sec * tv.tv_sec) + tv.tv_usec;
	return time_in_micros;
}

typedef struct sdl_texture_buffer {
	SDL_Texture *texture;
	pixel_buffer_s *pixel_buf;
} sdl_texture_buffer_s;

typedef struct sdl_event_context {
	sdl_texture_buffer_s *texture_buf;
	SDL_Window *window;
	Uint32 win_id;
	SDL_Renderer *renderer;
	SDL_Event *event;
	bool resized;
} sdl_event_context_s;

static void sdl_resize_texture_buf(SDL_Window *window,
				   SDL_Renderer *renderer,
				   sdl_texture_buffer_s *texture_buf)
{
	pixel_buffer_s *pixel_buf = texture_buf->pixel_buf;

	int height, width;
	SDL_GetWindowSize(window, &width, &height);

	if (texture_buf->texture && ((width == (int)pixel_buf->width)
				     && (height == (int)pixel_buf->height))) {
		/* nothing to do */
		return;
	}

	if (texture_buf->texture) {
		SDL_DestroyTexture(texture_buf->texture);
	}
	texture_buf->texture = SDL_CreateTexture(renderer,
						 SDL_PIXELFORMAT_ARGB8888,
						 SDL_TEXTUREACCESS_STREAMING,
						 width, height);
	if (!texture_buf->texture) {
		die("Could not alloc texture_buf->texture (%s)",
		    SDL_GetError());
	}

	if (!pixel_buffer_resize(pixel_buf, height, width)) {
		die("Could not resize pixel_buffer %d, %d", height, width);
	}
}

static void sdl_blit_bytes(SDL_Renderer *renderer, SDL_Texture *texture,
			   const void *pixels, int pitch)
{
	const SDL_Rect *rect = NULL;
	if (SDL_UpdateTexture(texture, rect, pixels, pitch)) {
		die("Could not SDL_UpdateTexture (%s)", SDL_GetError());
	}
	const SDL_Rect *srcrect = NULL;
	const SDL_Rect *dstrect = NULL;
	SDL_RenderCopy(renderer, texture, srcrect, dstrect);
	SDL_RenderPresent(renderer);
}

static void sdl_blit_texture(SDL_Renderer *renderer,
			     sdl_texture_buffer_s *texture_buf)
{
	SDL_Texture *texture = texture_buf->texture;
	const void *pixels = texture_buf->pixel_buf->pixels;
	int pitch = texture_buf->pixel_buf->pitch;

	sdl_blit_bytes(renderer, texture, pixels, pitch);
}

static void sdl_process_key_event(sdl_event_context_s *event_ctx,
				  human_input_s *input)
{
	int is_down = event_ctx->event->key.state == SDL_PRESSED;
	int was_down = ((event_ctx->event->key.repeat != 0)
			|| (event_ctx->event->key.state == SDL_RELEASED));

	switch (event_ctx->event->key.keysym.scancode) {
	case SDL_SCANCODE_ESCAPE:
		input->esc.is_down = is_down;
		input->esc.was_down = was_down;
		break;
	case SDL_SCANCODE_SPACE:
		input->space.is_down = is_down;
		input->space.was_down = was_down;
		break;
	case SDL_SCANCODE_UP:
		input->up.is_down = is_down;
		input->up.was_down = was_down;
		break;
	case SDL_SCANCODE_LEFT:
		input->left.is_down = is_down;
		input->left.was_down = was_down;
		break;
	case SDL_SCANCODE_DOWN:
		input->down.is_down = is_down;
		input->down.was_down = was_down;
		break;
	case SDL_SCANCODE_RIGHT:
		input->right.is_down = is_down;
		input->right.was_down = was_down;
		break;
	case SDL_SCANCODE_PAGEUP:
		input->page_up.is_down = is_down;
		input->page_up.was_down = was_down;
		break;
	case SDL_SCANCODE_PAGEDOWN:
		input->page_down.is_down = is_down;
		input->page_down.was_down = was_down;
		break;

	case SDL_SCANCODE_A:
		input->a.is_down = is_down;
		input->a.was_down = was_down;
		break;
	case SDL_SCANCODE_D:
		input->d.is_down = is_down;
		input->d.was_down = was_down;
		break;
	case SDL_SCANCODE_M:
		input->m.is_down = is_down;
		input->m.was_down = was_down;
		break;
	case SDL_SCANCODE_N:
		input->n.is_down = is_down;
		input->n.was_down = was_down;
		break;
	case SDL_SCANCODE_Q:
		input->q.is_down = is_down;
		input->q.was_down = was_down;
		break;
	case SDL_SCANCODE_S:
		input->s.is_down = is_down;
		input->s.was_down = was_down;
		break;
	case SDL_SCANCODE_W:
		input->w.is_down = is_down;
		input->w.was_down = was_down;
		break;
	case SDL_SCANCODE_X:
		input->x.is_down = is_down;
		input->x.was_down = was_down;
		break;
	case SDL_SCANCODE_Z:
		input->z.is_down = is_down;
		input->z.was_down = was_down;
		break;
	default:
		break;
	};
}

static void sdl_process_mouse_event(sdl_event_context_s *event_ctx,
				    human_input_s *input)
{
	int x, y;
	switch (event_ctx->event->type) {
	case SDL_MOUSEBUTTONDOWN:
		SDL_GetMouseState(&x, &y);
		input->click = 1;
		input->click_x = x;
		input->click_y = y;
		break;
	case SDL_MOUSEWHEEL:
		if (event_ctx->event->wheel.y > 0) {
			input->wheel_zoom = 1;
		} else if (event_ctx->event->wheel.y < 0) {
			input->wheel_zoom = -1;
		}
		break;
	default:
		break;
	}
}

static int sdl_process_event(sdl_event_context_s *event_ctx,
			     human_input_s *input)
{
	switch (event_ctx->event->type) {
	case SDL_QUIT:
		return 1;
		break;
	case SDL_KEYUP:
	case SDL_KEYDOWN:
		sdl_process_key_event(event_ctx, input);
		break;
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEWHEEL:
		sdl_process_mouse_event(event_ctx, input);
		break;
	case SDL_WINDOWEVENT:
		if (event_ctx->event->window.windowID != event_ctx->win_id) {
			/* not our event? */
			break;
		}
		switch (event_ctx->event->window.event) {
		case SDL_WINDOWEVENT_NONE:
			/* (docs say never used) */
			break;
		case SDL_WINDOWEVENT_SHOWN:
			break;
		case SDL_WINDOWEVENT_HIDDEN:
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			/* window should be redrawn */
			break;
		case SDL_WINDOWEVENT_MOVED:
			/* window moved to data1, data2 */
			break;
		case SDL_WINDOWEVENT_RESIZED:
			/* window resized to data1 x data2 */
			/* always preceded by */
			/* SDL_WINDOWEVENT_SIZE_CHANGED */
			sdl_resize_texture_buf(event_ctx->window,
					       event_ctx->renderer,
					       event_ctx->texture_buf);
			event_ctx->resized = true;
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			/* either as a result of an API call */
			/* or through the system
			   or user action */
			/* this event is followed by */
			/* SDL_WINDOWEVENT_RESIZED
			   if the size was */
			/* changed by an external event,
			   (user or the window manager) */
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			break;
		case SDL_WINDOWEVENT_RESTORED:
			break;
		case SDL_WINDOWEVENT_ENTER:
			/* window has gained mouse focus */
			break;
		case SDL_WINDOWEVENT_LEAVE:
			/* window has lost mouse focus */
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			/* window has gained keyboard focus */
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			/* window has lost keyboard focus */
			break;
		case SDL_WINDOWEVENT_CLOSE:
			/* the window manager requests close */
			event_ctx->event->type = SDL_QUIT;
			SDL_PushEvent(event_ctx->event);
			break;
		default:
			/* (how did we get here? */
			break;
		}
		break;
	default:
		break;
	}
	return 0;
}

void sdl_coord_plane_iteration(coordinate_plane_s *plane,
			       pixel_buffer_s *virtual_win)
{
	int window_x = plane->win_width;
	int window_y = plane->win_height;

	Uint32 init_flags = SDL_INIT_VIDEO;
	if (SDL_Init(init_flags) != 0) {
		die("Could not SDL_Init(%lu) (%s)", (uint64_t)init_flags,
		    SDL_GetError());
	}

	sdl_texture_buffer_s texture_buf;
	texture_buf.texture = NULL;
	texture_buf.pixel_buf = virtual_win;

	print_directions(plane, stdout);

	int x = SDL_WINDOWPOS_UNDEFINED;
	int y = SDL_WINDOWPOS_UNDEFINED;
	Uint32 win_flags = SDL_WINDOW_RESIZABLE;
	const char *title = pfuncs[plane->pfuncs_idx].name;
	SDL_Window *window =
	    SDL_CreateWindow(title, x, y, window_x, window_y, win_flags);
	if (!window) {
		die("Could not SDL_CreateWindow (%s)", SDL_GetError());
	}

	const int renderer_idx = -1;	// first renderer
	Uint32 rend_flags = 0;
	SDL_Renderer *renderer =
	    SDL_CreateRenderer(window, renderer_idx, rend_flags);
	if (!renderer) {
		die("Could not SDL_CreateRenderer (%s)", SDL_GetError());
	}

	sdl_resize_texture_buf(window, renderer, &texture_buf);

	SDL_Event event;
	sdl_event_context_s event_ctx;
	event_ctx.event = &event;
	event_ctx.texture_buf = &texture_buf;
	event_ctx.renderer = renderer;
	event_ctx.window = window;
	event_ctx.win_id = SDL_GetWindowID(window);
	event_ctx.resized = false;

	uint32_t it_per_frame = 1;
	uint64_t usec_per_sec = (1000 * 1000);
	uint64_t last_print = 0;
	uint64_t iterations_at_last_print = 0;
	uint64_t frames_since_print = 0;
	uint64_t frame_count = 0;

	human_input_s input[2];
	human_input_init(&input[0]);
	human_input_init(&input[1]);
	human_input_s *tmp_input = NULL;
	human_input_s *new_input = &input[1];
	human_input_s *old_input = &input[0];

	int shutdown = 0;
	while (!shutdown) {
		tmp_input = new_input;
		new_input = old_input;
		old_input = tmp_input;
		human_input_init(new_input);

		while (SDL_PollEvent(&event)) {
			shutdown = sdl_process_event(&event_ctx, new_input);
			if (shutdown) {
				break;
			}
		}
		if (shutdown) {
			break;
		}

		enum coordinate_plane_change change =
		    human_input_process(new_input, plane);
		if (change == coordinate_plane_change_shutdown) {
			shutdown = 1;
			break;
		}
		if (event_ctx.resized) {
			SDL_GetWindowSize(window, &window_x, &window_y);
			coordinate_plane_resize(plane, window_x, window_y);
			change = coordinate_plane_change_yes;
			event_ctx.resized = false;
		}
		if (change == coordinate_plane_change_yes) {
			iterations_at_last_print = 0;
			title = pfuncs[plane->pfuncs_idx].name;
			SDL_SetWindowTitle(window, title);
			print_directions(plane, stdout);
		}
		// set a 32 frames per second target
		uint64_t usec_per_frame_high_threshold = usec_per_sec / 30;
		uint64_t usec_per_frame_low_threshold = usec_per_sec / 45;
		uint64_t before = time_in_usec();

		coordinate_plane_iterate(plane, it_per_frame);
		pixel_buffer_update(plane, virtual_win);

		sdl_blit_texture(renderer, &texture_buf);
		++frame_count;
		++frames_since_print;

		uint64_t now = time_in_usec();
		uint64_t diff = now - before;
		if (diff < usec_per_frame_low_threshold) {
			++it_per_frame;
		} else if ((diff > usec_per_frame_high_threshold)
			   && (it_per_frame > 1)) {
			--it_per_frame;
		}

		uint64_t elapsed_since_last_print = now - last_print;
		if (elapsed_since_last_print > usec_per_sec) {
			double seconds_elapsed_since_last_print =
			    (1.0 * elapsed_since_last_print) / usec_per_sec;
			double fps =
			    frames_since_print /
			    seconds_elapsed_since_last_print;
			uint64_t it_count = plane->iteration_count;
			uint64_t it_diff;
			if (it_count >= iterations_at_last_print) {
				it_diff = it_count - iterations_at_last_print;
			} else {
				it_diff = it_count;
			}
			double ips = it_diff / seconds_elapsed_since_last_print;
			frames_since_print = 0;
			iterations_at_last_print = it_count;
			last_print = now;
			int fps_printer = 1;	// make configurable?
			if (fps_printer) {
				fprintf(stdout,
					"i:%" PRIu32 " escaped: %" PRIu64
					" not: %" PRIu64
					" (ips: %.f fps: %.f ipf: %" PRIu32
					" thds: %" PRIu32 ")     \r",
					plane->iteration_count, plane->escaped,
					plane->not_escaped, ips, fps,
					it_per_frame, plane->num_threads);
				fflush(stdout);
			}
		}
	}
	fprintf(stdout, "\n");

	/* we probably do not need to do these next steps */
	if (Make_valgrind_happy) {
		/* first cleanup SDL stuff */
		if (texture_buf.texture) {
			SDL_DestroyTexture(texture_buf.texture);
		}
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
}

int main(int argc, char **argv)
{

	coordinate_plane_s *plane = coordinate_plane_new_from_args(argc, argv);

	size_t palette_len = 1024;
	pixel_buffer_s *virtual_win =
	    pixel_buffer_new(plane->win_width, plane->win_height, palette_len,
			     plane->skip_rounds);

	sdl_coord_plane_iteration(plane, virtual_win);

	if (Make_valgrind_happy) {
		pixel_buffer_free(virtual_win);
		coordinate_plane_free(plane);
	}
}
