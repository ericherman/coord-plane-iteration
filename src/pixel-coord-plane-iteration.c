/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* pixel-coord-plane-iteration.h: playing with mandelbrot and such */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef SKIP_THREADS
#include <stdatomic.h>
#include "basic-thread-pool.h"
#endif

#include "alloc-or-die.h"
#include "coord-plane-option-parser.h"
#include "pixel-coord-plane-iteration.h"
#include "rgb-hsv.h"

static void long_tail_gradiant(struct rgb_24 *result, uint32_t distance)
{
	double hue, saturation, value;

	if (distance == 0) {
		hue = 0.0;
		saturation = 0.0;
		value = 0.0;
	} else {
		// factor should be 0.0 t/m 1.0
		double log_divisor = 8;
		double factor = fmod(log2(distance) / log_divisor, 1.0);
		assert(factor >= 0.0);
		assert(factor <= 1.0);

		hue = 360.0 * factor;
		saturation = 1.0;
		value = 1.0;
	}

#if DEBUG
	if (distance <= 10 || (distance % 100 == 0)) {
		fflush(stdout);
		fprintf(stderr, "\ni: %" PRIu32 ": hue: %g\n", distance, hue);
	}
#endif

	struct hsv_d hsv = { hue, saturation, value };
	struct rgb_d rgb = { 0.0, 0.0, 0.0 };
	rgb_d_from_hsv_d(&rgb, hsv);
	rgb_24_from_rgb_d(result, rgb);
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

enum coordinate_plane_change human_input_process(struct human_input *input,
						 struct coordinate_plane *plane)
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
		coordinate_plane_threads_more(plane);
		return coordinate_plane_change_no;
	}
	if (input->n.is_down && !input->n.was_down) {
		coordinate_plane_threads_less(plane);
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

void print_directions(struct coordinate_plane *plane, FILE *out)
{
	fprintf(out, "%s\n", coordinate_plane_function_name(plane));
	print_command_line(plane, out);
	fprintf(out, "use arrows or 'wasd' keys to pan\n");
	fprintf(out,
		"use page_down/page_up or 'z' and 'x' keys to zoom in/out\n");
	fprintf(out, "space will cycle through available functions\n");
	fprintf(out, "click to recenter the image\n");
	fprintf(out, "escape or 'q' to quit\n");
}

struct pixel_buffer_update_line_context {
	size_t id;
	struct coordinate_plane *plane;
	struct pixel_buffer *buf;
	uint32_t plane_win_width;
	uint32_t first_y;
	size_t lines;
#ifndef SKIP_THREADS
	atomic_bool done;
#else
	int done;
#endif
};

void pixel_buffer_update_line(struct coordinate_plane *plane,
			      struct pixel_buffer *buf,
			      uint32_t plane_win_width, uint32_t y)
{
	for (uint32_t x = 0; x < plane_win_width; x++) {
		size_t escaped = coordinate_plane_escaped(plane, x, y);
		size_t palette_idx = escaped % buf->palette_len;
		struct rgb_24 color = buf->palette[palette_idx];
		uint32_t foreground = rgb_24_to_uint32(color);
		*(buf->pixels + (y * buf->width) + x) = foreground;
	}
}

int pixel_buffer_update_line_ctx(void *context)
{
	struct pixel_buffer_update_line_context *ctx = context;

	struct coordinate_plane *plane = ctx->plane;
	struct pixel_buffer *buf = ctx->buf;
	uint32_t plane_win_width = ctx->plane_win_width;
	size_t lines = ctx->lines;
	uint32_t first_y = ctx->first_y;

	for (size_t i = 0; i < lines; ++i) {
		uint32_t y = first_y + i;
		pixel_buffer_update_line(plane, buf, plane_win_width, y);
	}

	ctx->done = true;

	return 0;
}

#ifndef SKIP_THREADS
static inline size_t minz(size_t a, size_t b)
{
	return a < b ? a : b;
}
#endif

void pixel_buffer_update(struct coordinate_plane *plane,
			 struct pixel_buffer *buf)
{
	uint32_t plane_win_width = coordinate_plane_win_width(plane);
	assert(plane_win_width > 0);
	if (plane_win_width != buf->width) {
		die("plane->win_width:%" PRIu32 " != buf->width: %" PRIu32,
		    plane_win_width, buf->width);
	}

	uint32_t plane_win_height = coordinate_plane_win_height(plane);
	assert(plane_win_height > 0);
	if (plane_win_height != buf->height) {
		die("plane->win_height:%" PRIu32 " != buf->height: %" PRIu32,
		    plane_win_height, buf->height);
	}
#ifndef SKIP_THREADS
	struct basic_thread_pool *tpool = coordinate_plane_thread_pool(plane);
	size_t thread_pool_size = basic_thread_pool_size(tpool);
	size_t line_ctx_size = sizeof(struct pixel_buffer_update_line_context);
	if (thread_pool_size < 2) {
#endif
		for (uint32_t y = 0; y < plane_win_height; y++) {
			pixel_buffer_update_line(plane, buf, plane_win_width,
						 y);
		}

#ifndef SKIP_THREADS
	} else {
		assert(basic_thread_pool_size(tpool));
		assert(basic_thread_pool_queue_size(tpool) == 0);

		if (!buf->contexts || buf->contexts_len < thread_pool_size) {
			if (buf->contexts) {
				free(buf->contexts);
			}
			buf->contexts = calloc_or_die("buf->contexts",
						      thread_pool_size,
						      line_ctx_size);
			buf->contexts_len = thread_pool_size;
		}

		assert(buf->contexts_len);

		struct pixel_buffer_update_line_context *ctx;
		size_t num_contexts = minz(buf->contexts_len, plane_win_height);
		size_t lines = plane_win_height / num_contexts;
		size_t leftover = plane_win_height % num_contexts;
		for (size_t i = 0; i < num_contexts; ++i) {

			ctx = buf->contexts + i;
			ctx->id = i;
			ctx->plane = plane;
			ctx->buf = buf;
			ctx->plane_win_width = plane_win_width;
			ctx->first_y = i * lines;
			ctx->lines = lines;
			if (i == num_contexts - 1) {
				ctx->lines = lines + leftover;
			} else {
				ctx->lines = lines;
			}
			ctx->done = 0;

			void *arg = ctx;
			thrd_start_t func = pixel_buffer_update_line_ctx;
			if (basic_thread_pool_add(tpool, func, arg)) {
				ctx->done = 1;
			}
		}
		for (size_t i = 0; i < num_contexts; ++i) {
			ctx = buf->contexts + i;
			while (!ctx->done) {
				thrd_yield();

			}
		}
	}
#endif
}

void *pixel_buffer_resize(struct pixel_buffer *buf, uint32_t height,
			  uint32_t width)
{
	assert(!height || ((unsigned long)width) <= SIZE_MAX / height);
	assert((width * (unsigned long)height) <=
	       (SIZE_MAX / sizeof(uint32_t)));

	if (buf->pixels) {
		free(buf->pixels);
	}
	buf->width = width;
	buf->height = height;

	buf->pixels_len = buf->height * buf->width;
	buf->pitch = buf->width * buf->bytes_per_pixel;

	size_t size = buf->pixels_len * buf->bytes_per_pixel;
	buf->pixels = malloc_or_die("buf->pixels", size);

	return buf->pixels;
}

static struct rgb_24 *grow_palette(struct rgb_24 *palette, size_t *len,
				   size_t prefix_black, size_t amount)
{
	size_t new_len = (amount + (*len));
	size_t size = sizeof(struct rgb_24) * new_len;
	struct rgb_24 *grow = realloc(palette, size);
	if (!grow) {
		errorf("could not allocate %zu bytes for palette[%zu]?",
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

struct pixel_buffer *pixel_buffer_new(uint32_t window_x, uint32_t window_y,
				      size_t palette_len, size_t skip_rounds)
{
	struct pixel_buffer *buf = NULL;
	size_t size = sizeof(struct pixel_buffer);
	buf = calloc_or_die("buf", 1, size);

	buf->bytes_per_pixel = sizeof(uint32_t);
	pixel_buffer_resize(buf, window_y, window_x);

	buf->palette = NULL;
	buf->palette_len = 0;

	buf->palette =
	    grow_palette(buf->palette, &buf->palette_len, skip_rounds,
			 palette_len);
	if (!buf->palette) {
		size_t size = sizeof(struct rgb_24) * palette_len;
		die("palette == NULL? (size %zu)", size);
	}

	return buf;
}

struct pixel_buffer *pixel_buffer_new_from_plane(struct coordinate_plane *plane,
						 size_t palette_len)
{
	uint32_t win_width = coordinate_plane_win_width(plane);
	uint32_t win_height = coordinate_plane_win_height(plane);
	uint32_t skip_rounds = coordinate_plane_skip_rounds(plane);
	struct pixel_buffer *buf =
	    pixel_buffer_new(win_width, win_height, palette_len, skip_rounds);
	return buf;
}

void pixel_buffer_free(struct pixel_buffer *buf)
{
	if (!buf) {
		return;
	}
	free(buf->palette);
	free(buf->pixels);
	free(buf->contexts);
	free(buf);
}
