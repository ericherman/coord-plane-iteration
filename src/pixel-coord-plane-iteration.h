/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* pixel-coord-plane-iteration.h: playing with mandlebrot and such */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#ifndef PIXEL_COORD_PLANE_INTERATION_H
#define PIXEL_COORD_PLANE_INTERATION_H 1

#include "rgb-hsv.h"
#include "coord-plane-iteration.h"

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

enum coordinate_plane_change {
	coordinate_plane_change_shutdown = -1,
	coordinate_plane_change_no = 0,
	coordinate_plane_change_yes = 1,
};

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
	struct pixel_buffer_update_line_context *contexts;
	size_t contexts_len;
} pixel_buffer_s;

void human_input_init(human_input_s *input);

enum coordinate_plane_change human_input_process(human_input_s *input,
						 coordinate_plane_s *plane);

void print_directions(coordinate_plane_s *plane, FILE *out);

void pixel_buffer_update(coordinate_plane_s *plane, pixel_buffer_s *buf);

void *pixel_buffer_resize(pixel_buffer_s *buf, uint32_t height, uint32_t width);

pixel_buffer_s *pixel_buffer_new(uint32_t window_x, uint32_t window_y,
				 size_t palette_len, size_t skip_rounds);

pixel_buffer_s *pixel_buffer_new_from_plane(coordinate_plane_s *plane,
					    size_t palette_len);

void pixel_buffer_free(pixel_buffer_s *buf);

#endif /* PIXEL_COORD_PLANE_INTERATION_H */
