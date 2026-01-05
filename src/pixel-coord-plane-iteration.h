/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* pixel-coord-plane-iteration.h: playing with mandelbrot and such */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#ifndef PIXEL_COORD_PLANE_INTERATION_H
#define PIXEL_COORD_PLANE_INTERATION_H 1

#include "rgb-hsv.h"
#include "coord-plane-iteration.h"

struct keyboard_key {
	uint32_t is_down:1;
	uint32_t was_down:1;
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

	struct keyboard_key m;
	struct keyboard_key n;

	struct keyboard_key q;
	struct keyboard_key space;
	struct keyboard_key esc;

	uint8_t click;
	uint32_t click_x;
	uint32_t click_y;

	int wheel_zoom;
};

enum coordinate_plane_change {
	coordinate_plane_change_shutdown = -1,
	coordinate_plane_change_no = 0,
	coordinate_plane_change_yes = 1,
};

struct pixel_buffer {
	uint32_t width;
	uint32_t height;
	uint8_t bytes_per_pixel;
	size_t pixels_len;
	/* pitch is bytes in a row of pixel data, including padding */
	uint32_t pitch;
	uint32_t *pixels;
	struct rgb_24 *palette;
	size_t palette_len;
	struct pixel_buffer_update_line_context *contexts;
	size_t contexts_len;
};

void human_input_init(struct human_input *input);

enum coordinate_plane_change human_input_process(struct human_input *input, struct coordinate_plane
						 *plane);

void print_directions(struct coordinate_plane *plane, FILE *out);

void pixel_buffer_update(struct coordinate_plane *plane,
			 struct pixel_buffer *buf);

void *pixel_buffer_resize(struct pixel_buffer *buf, uint32_t height,
			  uint32_t width);

struct pixel_buffer *pixel_buffer_new(uint32_t window_x, uint32_t window_y,
				      size_t palette_len, size_t skip_rounds);

struct pixel_buffer *pixel_buffer_new_from_plane(struct coordinate_plane *plane,
						 size_t palette_len);

void pixel_buffer_free(struct pixel_buffer *buf);

#endif /* PIXEL_COORD_PLANE_INTERATION_H */
