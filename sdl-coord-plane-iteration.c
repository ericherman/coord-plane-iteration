/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* sdl-coord-plane-iteration.c: playing with mandlebrot and such */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */
/*
   cc -g -O2 `sdl2-config --cflags` \
      ./sdl-coord-plane-iteration.c \
      -o ./sdl-coord-plane-iteration \
      `sdl2-config --libs` -lm &&
   ./sdl-coord-plane-iteration
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>

#ifndef Make_valgrind_happy
#define Make_valgrind_happy 0
#endif

#define die(format, ...) \
	do { \
		fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
		fprintf(stderr, format __VA_OPT__(,) __VA_ARGS__); \
		fprintf(stderr, "\n"); \
		exit(EXIT_FAILURE); \
	} while (0)

struct iterxy_s {
	/* coordinate location */
	long double cx;
	long double cy;

	/* calculated next location */
	long double zx;
	long double zy;

	unsigned iterations;

	unsigned escaped;
};

void ordinary_square(struct iterxy_s *p)
{
	if (p->escaped) {
		return;
	}
	long double escape_radius_squared = (2 * 2);
	if (((p->zy * p->zy) + (p->zx * p->zx)) > escape_radius_squared) {
		p->escaped = p->iterations;
	} else {
		p->zy = (p->zy == 0) ? p->cy : (p->zy * p->zy);
		p->zx = (p->zx == 0) ? p->cx : (p->zx * p->zx);
		++(p->iterations);
	}
}

/* Z[n+1] = (Z[n])^2 + Orig */
void mandlebrot(struct iterxy_s *p)
{
	if (p->escaped) {
		return;
	}
	long double escape_radius_squared = (2 * 2);
	if (((p->zy * p->zy) + (p->zx * p->zx)) > escape_radius_squared) {
		p->escaped = p->iterations;
	} else {
		/* first, square the complex */
		/* the y is understood to contain an i, the sqrt(-1) */
		/* generate and combine together the four combos */
		long double xx = p->zx * p->zx;	/* no i */
		long double yx = p->zy * p->zx;	/* has i */
		long double xy = p->zx * p->zy;	/* has i */
		long double yy = p->zy * p->zy * -1;	/* loses the i */

		long double result_x = xx + yy;	/* terms do not contain i */
		long double result_y = yx + xy;	/* terms contain an i */

		/* then add the original C to the Z[n]^2 result */
		p->zx = result_x + p->cx;
		p->zy = result_y + p->cy;

		++(p->iterations);
	}
}

/* Z[n+1] = collapse_to_y2_to_y((Z[n])^2) + Orig */
void square_binomial_collapse_y2_and_add_orig(struct iterxy_s *p)
{
	if (p->escaped) {
		return;
	}
	long double escape_radius_squared = (2 * 2);
	if (((p->zy * p->zy) + (p->zx * p->zx)) > escape_radius_squared) {
		p->escaped = p->iterations;
	} else {
		/* z[n+1] = z[n]^2 + B */

		/* squaring a binomial == adding together four combos */
		long double xx = p->zx * p->zx;
		long double yx = p->zy * p->zx;
		long double xy = p->zx * p->zy;
		long double yy = p->zy * p->zy;
		long double binomial_x = xx;	/* no y terms */
		long double collapse_y_and_y2_terms = yx + xy + yy;

		/* now add the original C */
		p->zx = binomial_x + p->cx;
		p->zy = collapse_y_and_y2_terms + p->cy;
		++(p->iterations);
	}
}

/* Z[n+1] = ignore_y2((Z[n])^2) + Orig */
void square_binomial_ignore_y2_and_add_orig(struct iterxy_s *p)
{
	if (p->escaped) {
		return;
	}
	long double escape_radius_squared = (2 * 2);
	if (((p->zy * p->zy) + (p->zx * p->zx)) > escape_radius_squared) {
		p->escaped = p->iterations;
	} else {
		/* z[n+1] = z[n]^2 + B */

		/* squaring a binomial == adding together four combos */
		long double xx = p->zx * p->zx;
		long double yx = p->zy * p->zx;
		long double xy = p->zx * p->zy;
		/*
		   long double yy = p->zy * p->zy;
		 */

		/* now add the original C */
		p->zx = xx + p->cx;
		p->zy = xy + yx + p->cy;
		++(p->iterations);
	}
}

void not_a_circle(struct iterxy_s *p)
{
	if (p->escaped) {
		return;
	}
	long double escape_radius_squared = (2 * 2);
	if (((p->zy * p->zy) + (p->zx * p->zx)) > escape_radius_squared) {
		p->escaped = p->iterations;
	} else {
		if (p->iterations == 0) {
			p->zy = p->cy;
			p->zx = p->cx;
		} else {
			long double xx = p->zx * p->zx;
			long double yy = p->zy * p->zy;
			p->zy = yy + (0.5 * p->zx);
			p->zx = xx + (0.5 * p->zy);
		}
		++(p->iterations);
	}
}

typedef void (*pfunc_f)(struct iterxy_s * p);

struct named_pfunc_s {
	pfunc_f pfunc;
	const char *name;
};

struct named_pfunc_s pfuncs[] = {
	{ mandlebrot, "mandlebrot" },
	{ ordinary_square, "ordinary_square" },
	{ not_a_circle, "not_a_circle" },
	{ square_binomial_collapse_y2_and_add_orig,
	 "square_binomial_collapse_y2_and_add_orig," },
	{ square_binomial_ignore_y2_and_add_orig,
	 "square_binomial_ignore_y2_and_add_orig," }
};

const size_t pfuncs_len = 5;

struct coordinate_plane_s {
	unsigned screen_width;
	unsigned screen_height;
	long double cx_min;
	long double cx_max;
	long double cy_min;
	long double cy_max;
	long double point_width;
	long double point_height;

	unsigned long iteration_count;
	unsigned long escaped;
	unsigned long not_escaped;

	size_t pfuncs_idx;

	struct iterxy_s *points;
	size_t len;
};

static inline long double _dmax(long double a, long double b)
{
	return a > b ? a : b;
}

struct coordinate_plane_s *coordinate_plane_reset(struct coordinate_plane_s
						  *plane, unsigned screen_width,
						  unsigned screen_height,
						  long double cx_min,
						  long double cx_max,
						  long double cy_min,
						  long double cy_max,
						  size_t pfuncs_idx)
{
	size_t needed = screen_width * screen_height;
	if (plane->points && (plane->len < needed)) {
		free(plane->points);
		plane->points = NULL;
	}
	if (!plane->points) {
		plane->len = needed;
		size_t size = plane->len * sizeof(struct iterxy_s);
		plane->points = calloc(1, size);
		if (!plane->points) {
			die("could not allocate %zu bytes?", size);
		}
	}

	plane->screen_width = screen_width;
	plane->screen_height = screen_height;

	plane->cx_min = cx_min;
	plane->cx_max = cx_max;
	long double coord_width = (plane->cx_max - plane->cx_min);
	plane->point_width = coord_width / screen_width;

	plane->cy_min = cy_min;
	plane->cy_max = cy_max;
	long double coord_height = (plane->cy_max - plane->cy_min);
	plane->point_height = coord_height / screen_height;

	plane->pfuncs_idx = pfuncs_idx;
	plane->iteration_count = 0;
	plane->escaped = 0;
	plane->not_escaped = (plane->screen_width * plane->screen_height);

	for (size_t py = 0; py < plane->screen_height; ++py) {
		for (size_t px = 0; px < plane->screen_width; ++px) {
			size_t i = (py * plane->screen_width) + px;
			struct iterxy_s *p = plane->points + i;

			/* location on the co-ordinate plane */
			p->cy = plane->cy_min + py * plane->point_height;
			if (fabsl(p->cy) < (plane->point_height / 2)) {
				/* near enought to zero to call it zero */
				p->cy = 0.0;
			}

			p->cx = plane->cx_min + px * plane->point_width;
			if (fabsl(p->cx) < (plane->point_width / 2)) {
				/* near enought to zero to call it zero */
				p->cx = 0.0;
			}

			p->zy = 0;
			p->zx = 0;
			p->iterations = 0;
			p->escaped = 0;
		}
	}
	return plane;
}

struct coordinate_plane_s *coordinate_plane_new(unsigned screen_width,
						unsigned screen_height,
						long double cx_min,
						long double cx_max,
						long double cy_min,
						long double cy_max,
						size_t pfunc_idx)
{
	size_t size = sizeof(struct coordinate_plane_s);
	struct coordinate_plane_s *plane = calloc(1, size);
	if (!plane) {
		die("could not allocate %zu bytes?", size);
	}

	coordinate_plane_reset(plane, screen_width, screen_height, cx_min,
			       cx_max, cy_min, cy_max, pfunc_idx);

	return plane;
}

void coordinate_plane_free(struct coordinate_plane_s *plane)
{
	if (plane) {
		free(plane->points);
	}
	free(plane);
}

size_t coordinate_plane_iterate(struct coordinate_plane_s *plane,
				unsigned times)
{
	pfunc_f pfunc = pfuncs[plane->pfuncs_idx].pfunc;

	size_t old_escaped = plane->escaped;

	for (size_t i = 0; i < times; ++i) {
		++(plane->iteration_count);
		plane->escaped = 0;
		plane->not_escaped = plane->len;
		for (size_t j = 0; j < plane->len; ++j) {
			struct iterxy_s *p = plane->points + j;

			pfunc(p);

			if (p->escaped) {
				++(plane->escaped);
				--(plane->not_escaped);
			}
		}
	}

	return plane->escaped - old_escaped;
}

void coordinate_plane_next_function(struct coordinate_plane_s *plane)
{
	size_t pfuncs_idx = 1 + plane->pfuncs_idx;
	if (pfuncs_idx >= pfuncs_len) {
		pfuncs_idx = 0;
	}
	coordinate_plane_reset(plane, plane->screen_width, plane->screen_height,
			       plane->cx_min, plane->cx_max, plane->cy_min,
			       plane->cy_max, pfuncs_idx);
}

void coordinate_plane_zoom_in(struct coordinate_plane_s *plane)
{
	long double x_diff = plane->cx_max - plane->cx_min;
	long double y_diff = plane->cy_max - plane->cy_min;
	long double x_zoom_diff = x_diff / 8;
	long double y_zoom_diff = y_diff / 8;

	long double cx_min = plane->cx_min + x_zoom_diff;
	long double cx_max = plane->cx_max - x_zoom_diff;
	long double cy_min = plane->cy_min + y_zoom_diff;
	long double cy_max = plane->cy_max - y_zoom_diff;

	coordinate_plane_reset(plane, plane->screen_width, plane->screen_height,
			       cx_min, cx_max, cy_min, cy_max,
			       plane->pfuncs_idx);
}

void coordinate_plane_zoom_out(struct coordinate_plane_s *plane)
{
	long double x_diff = plane->cx_max - plane->cx_min;
	long double y_diff = plane->cy_max - plane->cy_min;
	long double x_zoom_diff = x_diff / 8;
	long double y_zoom_diff = y_diff / 8;

	long double cx_min = plane->cx_min - x_zoom_diff;
	long double cx_max = plane->cx_max + x_zoom_diff;
	long double cy_min = plane->cy_min - y_zoom_diff;
	long double cy_max = plane->cy_max + y_zoom_diff;

	coordinate_plane_reset(plane, plane->screen_width, plane->screen_height,
			       cx_min, cx_max, cy_min, cy_max,
			       plane->pfuncs_idx);
}

void coordinate_plane_pan_left(struct coordinate_plane_s *plane)
{
	long double x_diff = plane->cx_max - plane->cx_min;
	long double y_diff = plane->cy_max - plane->cy_min;
	long double x_pan_diff = x_diff / 4;

	long double cx_min = plane->cx_min - x_pan_diff;
	long double cx_max = plane->cx_max - x_pan_diff;

	coordinate_plane_reset(plane, plane->screen_width, plane->screen_height,
			       cx_min, cx_max, plane->cy_min, plane->cy_max,
			       plane->pfuncs_idx);
}

void coordinate_plane_pan_right(struct coordinate_plane_s *plane)
{
	long double x_diff = plane->cx_max - plane->cx_min;
	long double y_diff = plane->cy_max - plane->cy_min;
	long double x_pan_diff = x_diff / 4;

	long double cx_min = plane->cx_min + x_pan_diff;
	long double cx_max = plane->cx_max + x_pan_diff;

	coordinate_plane_reset(plane, plane->screen_width, plane->screen_height,
			       cx_min, cx_max, plane->cy_min, plane->cy_max,
			       plane->pfuncs_idx);
}

void coordinate_plane_pan_up(struct coordinate_plane_s *plane)
{
	long double y_diff = plane->cy_max - plane->cy_min;
	long double y_pan_diff = y_diff / 4;

	long double cy_max = plane->cy_max - y_pan_diff;
	long double cy_min = plane->cy_min - y_pan_diff;

	coordinate_plane_reset(plane, plane->screen_width, plane->screen_height,
			       plane->cx_min, plane->cx_max, cy_min, cy_max,
			       plane->pfuncs_idx);
}

void coordinate_plane_pan_down(struct coordinate_plane_s *plane)
{
	long double y_diff = plane->cy_max - plane->cy_min;
	long double y_pan_diff = y_diff / 4;

	long double cy_max = plane->cy_max + y_pan_diff;
	long double cy_min = plane->cy_min + y_pan_diff;

	coordinate_plane_reset(plane, plane->screen_width, plane->screen_height,
			       plane->cx_min, plane->cx_max, cy_min, cy_max,
			       plane->pfuncs_idx);
}

struct pixel_buffer {
	unsigned int width;
	unsigned int height;
	unsigned char bytes_per_pixel;
	size_t pixels_len;
	/* pitch is bytes in a row of pixel data, including padding */
	unsigned int pitch;
	unsigned int *pixels;
	struct rgb24_s *palette;
	size_t palette_len;

};

struct keyboard_key {
	unsigned int is_down:1;
	unsigned int was_down:1;
};

struct human_input {
	struct keyboard_key up;
	struct keyboard_key w;

	struct keyboard_key left;
	struct keyboard_key a;

	struct keyboard_key down;
	struct keyboard_key s;

	struct keyboard_key right;
	struct keyboard_key d;

	struct keyboard_key page_up;
	struct keyboard_key z;

	struct keyboard_key page_down;
	struct keyboard_key x;

	struct keyboard_key q;
	struct keyboard_key space;
	struct keyboard_key esc;
};

struct rgb24_s {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

// values are from 0.0 to 1.0
struct rgb_s {
	double red;
	double green;
	double blue;
};

// hue is 0.0 to 360.0
// saturation and value are 0.0 to 0.1
struct hsv_s {
	double hue;
	double sat;
	double val;
};

int invalid_hsv_s(struct hsv_s hsv)
{
	if (!(hsv.hue >= 0.0 && hsv.hue <= 360.0)) {
		return 1;
	}
	if (!(hsv.sat >= 0 && hsv.sat <= 1.0)) {
		return 1;
	}
	if (!(hsv.val >= 0 && hsv.val <= 1.0)) {
		return 1;
	}
	return 0;
}

void rgb24_from_rgb(struct rgb24_s *out, struct rgb_s in)
{
	out->red = 255 * in.red;
	out->green = 255 * in.green;
	out->blue = 255 * in.blue;
}

static inline unsigned int rgb24_to_uint(struct rgb24_s rgb)
{
	unsigned int urgb = ((rgb.red << 16) + (rgb.green << 8) + rgb.blue);
	return urgb;
}

// https://dystopiancode.blogspot.com/2012/06/hsv-rgb-conversion-algorithms-in-c.html
// https://en.wikipedia.org/wiki/HSL_and_HSV
int rgb_from_hsv(struct rgb_s *rgb, struct hsv_s hsv)
{
	if (!rgb || invalid_hsv_s(hsv)) {
		return 1;
	}

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

	return 0;
}

void bright_palette_range(double from, double to, struct rgb24_s *palette,
			  size_t len)
{
	assert(from < to);
	assert(from >= 0.0);
	assert(to <= 360.);

	double distance = to - from;
	struct rgb_s tmp;
	for (size_t i = 0; i < len; ++i) {
		double hue = from + (distance - ((distance / len) * i));
		if (hue > 360.0) {
			hue = fmod(hue, 360.0);
		}
		struct hsv_s hsv;
		hsv.hue = hue;
		hsv.sat = 1.0;
		hsv.val = 1.0;
		rgb_from_hsv(&tmp, hsv);
		rgb24_from_rgb(palette + i, tmp);
	}
}

void bright_palette(struct rgb24_s *palette, size_t len)
{
	bright_palette_range(0.0, 360.0, palette, len);
}

unsigned int rgb_for_escape(unsigned int highlight_latest,
			    unsigned int iterations, unsigned int escaped,
			    struct rgb24_s *palette, size_t len)
{
	if (escaped == 0) {
		return 0;
	}

	size_t palette_idx;
	if ((highlight_latest) && (escaped <= len / 3)) {
		palette_idx = escaped;
	} else if ((highlight_latest)
		   && (((1.0 * escaped) / iterations) < 0.75)) {
		palette_idx =
		    (len / 3) + (((1.0 * escaped) / iterations) * (len / 3));
	} else {
		palette_idx = (escaped % len);
	}

	unsigned int urgb = rgb24_to_uint(palette[palette_idx]);
	return urgb;
}

void pixel_buffer_update(struct coordinate_plane_s *plane,
			 struct pixel_buffer *buf,
			 unsigned int highlight_latest)
{
	if (plane->screen_width != buf->width) {
		die("plane->screen_width:%u != buf->width: %u",
		    plane->screen_width, buf->width);
	}
	if (plane->screen_height != buf->height) {
		die("plane->screen_height:%u != buf->height: %u",
		    plane->screen_height, buf->height);
	}

	for (unsigned int y = 0; y < plane->screen_height; y++) {
		for (unsigned int x = 0; x < plane->screen_width; x++) {
			size_t i = (y * plane->screen_width) + x;
			struct iterxy_s *p = plane->points + i;
			unsigned int foreground =
			    rgb_for_escape(highlight_latest,
					   plane->iteration_count, p->escaped,
					   buf->palette, buf->palette_len);
			*(buf->pixels + (y * buf->width) + x) = foreground;
		}
	}
}

int human_input_process(struct human_input *input,
			struct coordinate_plane_s *plane)
{
	if ((input->esc.is_down) || (input->q.is_down)) {
		return -1;
	}

	if (input->space.is_down) {
		coordinate_plane_next_function(plane);
		return 1;
	}

	if ((input->w.is_down && !input->w.was_down) ||
	    (input->up.is_down && !input->up.was_down)) {
		coordinate_plane_pan_up(plane);
		return 1;
	}
	if ((input->s.is_down && !input->s.was_down) ||
	    (input->down.is_down && !input->down.was_down)) {
		coordinate_plane_pan_down(plane);
		return 1;
	}

	if ((input->a.is_down && !input->a.was_down) ||
	    (input->left.is_down && !input->left.was_down)) {
		coordinate_plane_pan_left(plane);
		return 1;
	}
	if ((input->d.is_down && !input->d.was_down) ||
	    (input->right.is_down && !input->right.was_down)) {
		coordinate_plane_pan_right(plane);
		return 1;
	}
	if ((input->x.is_down && !input->x.was_down) ||
	    (input->page_up.is_down && !input->page_up.was_down)) {
		coordinate_plane_zoom_out(plane);
		return 1;
	}
	if ((input->z.is_down && !input->z.was_down) ||
	    (input->page_down.is_down && !input->page_down.was_down)) {
		coordinate_plane_zoom_in(plane);
		return 1;
	}

	return 0;
}

static void *pixel_buffer_resize(struct pixel_buffer *buf, int height,
				 int width)
{
	if (buf->pixels) {
		free(buf->pixels);
	}
	buf->width = width;
	buf->height = height;
	buf->pixels_len = buf->height * buf->width;
	buf->pitch = buf->width * buf->bytes_per_pixel;
	size_t size = buf->pixels_len * buf->bytes_per_pixel;
	buf->pixels = calloc(1, size);
	if (!buf->pixels) {
		die("Could not alloc buf->pixels (%d)", size);
	}
	return buf->pixels;
}

static struct pixel_buffer *pixel_buffer_new(unsigned int window_x,
					     unsigned int window_y,
					     size_t palette_len)
{
	size_t size = sizeof(struct pixel_buffer);
	struct pixel_buffer *buf = calloc(1, size);
	if (!buf) {
		die("could not allocate %lu bytes?", size);
	}

	buf->bytes_per_pixel = sizeof(unsigned int);
	pixel_buffer_resize(buf, window_y, window_x);

	buf->palette_len = palette_len;
	size = buf->palette_len * sizeof(struct rgb24_s);
	buf->palette = calloc(1, size);
	if (!buf->palette) {
		die("could not allocate %zu bytes?", size);
	}

	bright_palette(buf->palette, palette_len);

	return buf;
}

void pixel_buffer_free(struct pixel_buffer *buf)
{
	if (!buf) {
		return;
	}
	free(buf->palette);
	free(buf->pixels);
	free(buf);
}

static void diff_timespecs(struct timespec *elapsed, struct timespec start,
			   struct timespec end)
{
	if ((end.tv_nsec - start.tv_nsec) < 0) {
		elapsed->tv_sec = end.tv_sec - start.tv_sec - 1;
		elapsed->tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		elapsed->tv_sec = end.tv_sec - start.tv_sec;
		elapsed->tv_nsec = end.tv_nsec - start.tv_nsec;
	}
}

void human_input_init(struct human_input *input)
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

	input->q.is_down = 0;
	input->q.was_down = 0;

	input->space.is_down = 0;
	input->space.was_down = 0;

	input->esc.is_down = 0;
	input->esc.was_down = 0;
}

void print_directions(struct coordinate_plane_s *plane)
{
	const char *title = pfuncs[plane->pfuncs_idx].name;
	printf("\n\n%s\n", title);
	printf("use arrows or 'wasd' keys to pan\n");
	printf("use page_down/page_up or 'z' and 'x' keys to zoom in/out\n");
	printf("space will cycle through available functions\n");
	printf("escape or 'q' to quit\n");
	long double cx_min = plane->cx_min;
	long double cx_max = plane->cx_min;
	printf("x-axis co-ordinates range from: %Lf to: %Lf\n", cx_min, cx_max);
	long double cy_min = plane->cy_min;
	long double cy_max = plane->cy_min;
	printf("y-axis co-ordinates range from: %Lf to: %Lf\n", cy_min, cy_max);
}

struct sdl_texture_buffer {
	SDL_Texture *texture;
	struct pixel_buffer *pixel_buf;
};

struct sdl_event_context {
	struct sdl_texture_buffer *texture_buf;
	SDL_Window *window;
	Uint32 win_id;
	SDL_Renderer *renderer;
	SDL_Event *event;
	int resized;
};

static void sdl_resize_texture_buf(SDL_Window *window,
				   SDL_Renderer *renderer,
				   struct sdl_texture_buffer *texture_buf)
{
	struct pixel_buffer *pixel_buf = texture_buf->pixel_buf;

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
		die("Could not resize pixel_buffer");
	}
}

static void sdl_blit_bytes(SDL_Renderer *renderer, SDL_Texture *texture,
			   const SDL_Rect * rect, const void *pixels, int pitch)
{
	if (SDL_UpdateTexture(texture, rect, pixels, pitch)) {
		die("Could not SDL_UpdateTexture (%s)", SDL_GetError());
	}
	SDL_RenderCopy(renderer, texture, 0, 0);
	SDL_RenderPresent(renderer);
}

static void sdl_blit_texture(SDL_Renderer *renderer,
			     struct sdl_texture_buffer *texture_buf)
{
	SDL_Texture *texture = texture_buf->texture;
	const SDL_Rect *rect = 0;
	const void *pixels = texture_buf->pixel_buf->pixels;
	int pitch = texture_buf->pixel_buf->pitch;

	sdl_blit_bytes(renderer, texture, rect, pixels, pitch);
}

static void sdl_process_key_event(struct sdl_event_context *event_ctx,
				  struct human_input *input)
{
	int is_down, was_down;

	is_down = event_ctx->event->key.state == SDL_PRESSED;
	was_down = ((event_ctx->event->key.repeat != 0)
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

static int sdl_process_event(struct sdl_event_context *event_ctx,
			     struct human_input *input)
{
	switch (event_ctx->event->type) {
	case SDL_QUIT:
		return 1;
		break;
	case SDL_KEYUP:
	case SDL_KEYDOWN:
		sdl_process_key_event(event_ctx, input);
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
			event_ctx->resized = 1;
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

int main(int argc, const char **argv)
{
	int window_x = argc > 1 ? atoi(argv[1]) : 800;
	int window_y = argc > 2 ? atoi(argv[2]) : (window_x * 3) / 4;

	size_t palette_len = 15;

	/* define the range of the X dimension */
	long double cx_min = -2.5;
	long double cx_max = 1.5;
	if (argc > 3) {
		sscanf(argv[3], "%Lf", &cx_min);
		if (argc > 4) {
			sscanf(argv[4], "%Lf", &cx_max);
		}
	}
	long double coord_width = cx_max - cx_min;
	long double cy_min = (coord_width * ((1.0 * window_y) / window_x)) / 2;
	long double cy_max = -cy_min;

	unsigned int func_idx = argc > 5 ? (unsigned)atoi(argv[5]) : 0U;
	unsigned int highlight_latest = argc > 6 ? (unsigned)atoi(argv[6]) : 0U;

	if (window_x <= 0 || window_y <= 0) {
		die("window_x = %d\nwindow_y = %d\n", window_x, window_y);
	}
	if (func_idx >= pfuncs_len) {
		func_idx = 0;
	}

	Uint32 flags = SDL_INIT_VIDEO;
	if (SDL_Init(flags) != 0) {
		die("Could not SDL_Init(%lu) (%s)", (unsigned long)flags,
		    SDL_GetError());
	}

	struct pixel_buffer *virtual_win =
	    pixel_buffer_new(window_x, window_y, palette_len);

	struct sdl_texture_buffer texture_buf;
	texture_buf.texture = NULL;
	texture_buf.pixel_buf = virtual_win;

	struct coordinate_plane_s *plane =
	    coordinate_plane_new(window_x, window_y, cx_min, cx_max, cy_min,
				 cy_max, func_idx);
	print_directions(plane);

	int x = SDL_WINDOWPOS_UNDEFINED;
	int y = SDL_WINDOWPOS_UNDEFINED;
	flags = SDL_WINDOW_RESIZABLE;
	const char *title = pfuncs[plane->pfuncs_idx].name;
	SDL_Window *window =
	    SDL_CreateWindow(title, x, y, window_x, window_y, flags);
	if (!window) {
		die("Could not SDL_CreateWindow (%s)", SDL_GetError());
	}

	const int first = -1;
	const int none = 0;
	SDL_Renderer *renderer = SDL_CreateRenderer(window, first, none);
	if (!renderer) {
		die("Could not SDL_CreateRenderer (%s)", SDL_GetError());
	}

	sdl_resize_texture_buf(window, renderer, &texture_buf);

	SDL_Event event;
	struct sdl_event_context event_ctx;
	event_ctx.event = &event;
	event_ctx.texture_buf = &texture_buf;
	event_ctx.renderer = renderer;
	event_ctx.window = window;
	event_ctx.win_id = SDL_GetWindowID(window);
	event_ctx.resized = 0;

	struct timespec last_print;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &last_print);

	unsigned long frames_since_print = 0;
	unsigned long frame_count = 0;

	struct human_input input[2];
	human_input_init(&input[0]);
	human_input_init(&input[1]);
	struct human_input *tmp_input = NULL;
	struct human_input *new_input = &input[1];
	struct human_input *old_input = &input[0];

	int shutdown = 0;
	int iterations_per_loop = 1;
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

		int change = human_input_process(new_input, plane);
		if (change == -1) {
			shutdown = 1;
			break;
		}
		if (event_ctx.resized) {
			SDL_GetWindowSize(window, &window_x, &window_y);
			coordinate_plane_reset(plane, window_x,
					       window_y,
					       plane->cx_min,
					       plane->cx_max,
					       plane->cy_min,
					       plane->cy_max,
					       plane->pfuncs_idx);
			change = 1;
			event_ctx.resized = 0;
		}
		if (change) {
			title = pfuncs[plane->pfuncs_idx].name;
			SDL_SetWindowTitle(window, title);
			print_directions(plane);
		}
		coordinate_plane_iterate(plane, iterations_per_loop);
		pixel_buffer_update(plane, virtual_win, highlight_latest);
		sdl_blit_texture(renderer, &texture_buf);

		frame_count++;
		frames_since_print++;

		struct timespec now;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);

		struct timespec elapsed;
		diff_timespecs(&elapsed, last_print, now);

		if (((elapsed.tv_sec * 1000000000) + elapsed.tv_nsec) >
		    100000000) {
			double fps =
			    ((double)frames_since_print) /
			    (((double)elapsed.tv_sec) * 1000000000.0 +
			     ((double)elapsed.tv_nsec) / 1000000000.0);
			frames_since_print = 0;
			last_print.tv_sec = now.tv_sec;
			last_print.tv_nsec = now.tv_nsec;
			if (fps > 50.0) {
				++iterations_per_loop;
			} else if (fps < 40.0 && iterations_per_loop > 1) {
				--iterations_per_loop;
			}
			int fps_printer = 1;	// make configurable?
			if (fps_printer) {
				fprintf(stdout,
					"%lu, "
					"escaped: %lu, not escaped: %lu "
					"(fps: %.f ipl: %u)  \r",
					plane->iteration_count, plane->escaped,
					plane->not_escaped, fps,
					iterations_per_loop);
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

		/* then collect our own garbage */
		coordinate_plane_free(plane);
		pixel_buffer_free(virtual_win);
	}

	return 0;
}
