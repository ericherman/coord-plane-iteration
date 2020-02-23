/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* sdl-coord-plane-iteration.c: playing with mandlebrot and such */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */
/*
   cc -g -O2 `sdl2-config --cflags` \
      ./sdl-coord-plane-iteration.c \
      -o ./sdl-coord-plane-iteration \
      `sdl2-config --libs` &&
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

typedef void (*pfunc_f)(struct iterxy_s * p);

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

struct coordinate_plane_s {
	unsigned screen_width;
	unsigned screen_height;
	long double cx_min;
	long double cx_max;
	long double coord_width;
	long double cy_min;
	long double cy_max;
	long double coord_height;
	long double point_width;
	long double point_height;

	struct iterxy_s *points;
	size_t len;
};

static inline long double _dmax(long double a, long double b)
{
	return a > b ? a : b;
}

struct coordinate_plane_s *new_coordinate_plane(unsigned screen_width,
						unsigned screen_height,
						long double cx_min,
						long double cx_max)
{
	size_t size = sizeof(struct coordinate_plane_s);
	struct coordinate_plane_s *plane = malloc(size);
	if (!plane) {
		die("could not allocate %zu bytes?", size);
	}

	plane->cx_min = cx_min;
	plane->cx_max = cx_max;
	plane->screen_width = screen_width;
	plane->screen_height = screen_height;
	plane->coord_width = (plane->cx_max - plane->cx_min);
	plane->cy_min =
	    _dmax(cx_max,
		  ((plane->coord_width) *
		   ((1.0 * screen_height) / screen_width)) / 2);
	plane->cy_max = -(plane->cy_min);
	plane->coord_height = (plane->cy_max - plane->cy_min);
	plane->point_width = plane->coord_width / screen_width;
	plane->point_height = plane->coord_height / screen_height;
	plane->points = NULL;
	plane->len = plane->screen_width * plane->screen_height;

	size = plane->len * sizeof(struct iterxy_s);
	plane->points = malloc(size);
	if (!plane->points) {
		die("could not allocate %zu bytes?", size);
	}
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

void delete_coordinate_plane(struct coordinate_plane_s *plane)
{
	if (plane) {
		free(plane->points);
	}
	free(plane);
}

void iterate_plane(struct coordinate_plane_s *plane, pfunc_f pfunc)
{
	for (size_t j = 0; j < plane->len; ++j) {
		struct iterxy_s *p = plane->points + j;
		pfunc(p);
	}
}

struct named_pfunc_s {
	pfunc_f pfunc;
	const char *name;
};

struct pixel_buffer {
	unsigned int width;
	unsigned int height;
	unsigned char bytes_per_pixel;
	unsigned int pixels_bytes_len;
	/* pitch is bytes in a row of pixel data, including padding */
	unsigned int pitch;
	unsigned int *pixels;
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

	struct keyboard_key q;
	struct keyboard_key space;
	struct keyboard_key esc;
};

struct rgb_s {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

unsigned int rgb_for_escape(unsigned int iterations)
{
	if (iterations == 0) {
		return 0;
	}
	struct rgb_s pallet[] = {
		{ 255, 0, 0 },
		{ 0, 255, 0 },
		{ 0, 0, 255 },

		{ 255, 255, 0 },
		{ 0, 255, 255 },
		{ 255, 0, 255 },

		{ 255, 127, 0 },
		{ 255, 0, 127 },
		{ 0, 255, 127 },
		{ 127, 255, 0 },
		{ 127, 0, 255 },
		{ 0, 127, 255 },

		{ 127, 127, 0 },
		{ 127, 0, 127 },
		{ 0, 127, 127 },
		{ 127, 127, 0 },
		{ 127, 0, 127 },
		{ 0, 127, 127 }
	};

	struct rgb_s rgb = pallet[iterations % 18];

	unsigned int urgb = ((rgb.r << 16) + (rgb.g << 8) + rgb.b);

	return urgb;
}

void update_pixel_buffer(struct coordinate_plane_s *plane,
			 struct pixel_buffer *virtual_win,
			 unsigned long *escaped, unsigned long *not_escaped)
{
	assert(plane->screen_width == virtual_win->width);
	assert(plane->screen_height == virtual_win->height);

	*escaped = 0;
	*not_escaped = 0;

	for (unsigned int y = 0; y < plane->screen_height; y++) {
		for (unsigned int x = 0; x < plane->screen_width; x++) {
			size_t i = (y * plane->screen_width) + x;
			struct iterxy_s *p = plane->points + i;
			unsigned int foreground = rgb_for_escape(p->escaped);
			if (p->escaped) {
				++(*escaped);
			} else {
				++(*not_escaped);
			}
			*(virtual_win->pixels + (y * virtual_win->width) + x) =
			    foreground;
		}
	}
}

int process_input(struct human_input *input, int *press)
{
	if ((input->esc.is_down) || (input->q.is_down)) {
		return 1;
	}

	if (input->space.is_down) {
		*press = ' ';
		return 0;
	}

	if ((input->w.is_down && !input->w.was_down) ||
	    (input->up.is_down && !input->up.was_down)) {
		*press = 'w';
		return 0;
	}
	if ((input->s.is_down && !input->s.was_down) ||
	    (input->down.is_down && !input->down.was_down)) {
		*press = 's';
		return 0;
	}

	if ((input->a.is_down && !input->a.was_down) ||
	    (input->left.is_down && !input->left.was_down)) {
		*press = 'a';
		return 0;
	}
	if ((input->d.is_down && !input->d.was_down) ||
	    (input->right.is_down && !input->right.was_down)) {
		*press = 'd';
		return 0;
	}

	return 0;
}

static void *resize_pixel_buffer(struct pixel_buffer *buf, int height,
				 int width)
{
	if (buf->pixels) {
		free(buf->pixels);
	}
	buf->width = width;
	buf->height = height;
	buf->pixels_bytes_len = buf->height * buf->width * buf->bytes_per_pixel;
	buf->pitch = buf->width * buf->bytes_per_pixel;
	buf->pixels = malloc(buf->pixels_bytes_len);
	if (!buf->pixels) {
		die("Could not alloc buf->pixels (%d)", buf->pixels_bytes_len);
	}
	return buf->pixels;
}

static void pixel_buffer_init(struct pixel_buffer *buf)
{
	buf->width = 0;
	buf->height = 0;
	buf->bytes_per_pixel = sizeof(unsigned int);
	buf->pixels = NULL;
	buf->pixels_bytes_len = 0;
	buf->pitch = 0;
}

static void diff_timespecs(struct timespec start, struct timespec end,
			   struct timespec *elapsed)
{
	if ((end.tv_nsec - start.tv_nsec) < 0) {
		elapsed->tv_sec = end.tv_sec - start.tv_sec - 1;
		elapsed->tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		elapsed->tv_sec = end.tv_sec - start.tv_sec;
		elapsed->tv_nsec = end.tv_nsec - start.tv_nsec;
	}
}

void init_input(struct human_input *input)
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

	input->q.is_down = 0;
	input->q.was_down = 0;

	input->space.is_down = 0;
	input->space.was_down = 0;

	input->esc.is_down = 0;
	input->esc.was_down = 0;
}

void print_directions(const char *title, long double cx_min, long double cx_max)
{
	printf("\n\n%s\n", title);
	printf("use arrows or 'wasd' keys to zoom and pan\n");
	printf("space will cycle through available functions\n");
	printf("escape or 'q' to quit\n");
	printf("x-axis co-ordinates range from: %Lf to: %Lf\n", cx_min, cx_max);
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

static void sdl_resize_texture_buf(SDL_Window * window,
				   SDL_Renderer * renderer,
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

	if (!resize_pixel_buffer(pixel_buf, height, width)) {
		die("Could not resize pixel_buffer");
	}
}

static void sdl_blit_bytes(SDL_Renderer * renderer, SDL_Texture * texture,
			   const SDL_Rect * rect, const void *pixels, int pitch)
{
	if (SDL_UpdateTexture(texture, rect, pixels, pitch)) {
		die("Could not SDL_UpdateTexture (%s)", SDL_GetError());
	}
	SDL_RenderCopy(renderer, texture, 0, 0);
	SDL_RenderPresent(renderer);
}

static void sdl_blit_texture(SDL_Renderer * renderer,
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
	struct named_pfunc_s nf[] = {
		{ mandlebrot, "mandlebrot" },
		{ ordinary_square, "ordinary_square" },
		{ not_a_circle, "not_a_circle" },
		{ square_binomial_collapse_y2_and_add_orig,
		 "square_binomial_collapse_y2_and_add_orig," },
		{ square_binomial_ignore_y2_and_add_orig,
		 "square_binomial_ignore_y2_and_add_orig," }
	};

	int window_x = argc > 1 ? atoi(argv[1]) : 800;
	int window_y = argc > 2 ? atoi(argv[2]) : (window_x * 3) / 4;

	/* define the range of the X dimension */
	long double cx_min = -2.5;
	long double cx_max = 1.5;
	if (argc > 3) {
		sscanf(argv[3], "%Lf", &cx_min);
		if (argc > 4) {
			sscanf(argv[4], "%Lf", &cx_max);
		}
	}

	int func_idx = argc > 5 ? atoi(argv[5]) : 0;

	if (window_x <= 0 || window_y <= 0) {
		die("window_x = %d\nwindow_y = %d\n", window_x, window_y);
	}
	if (func_idx < 0 || func_idx > 4) {
		func_idx = 0;
	}
	const char *title = nf[func_idx].name;

	Uint32 flags = SDL_INIT_VIDEO;
	if (SDL_Init(flags) != 0) {
		die("Could not SDL_Init(%lu) (%s)", (unsigned long)flags,
		    SDL_GetError());
	}

	size_t size = sizeof(struct pixel_buffer);
	struct pixel_buffer *virtual_win = malloc(size);
	if (!virtual_win) {
		die("could not allocate %lu bytes?", size);
	}
	pixel_buffer_init(virtual_win);

	virtual_win->width = window_x;
	virtual_win->height = window_y;
	virtual_win->bytes_per_pixel = sizeof(unsigned int);
	virtual_win->pitch =
	    (virtual_win->width * virtual_win->bytes_per_pixel);
	virtual_win->pixels_bytes_len =
	    (virtual_win->width * virtual_win->height *
	     virtual_win->bytes_per_pixel);
	size = virtual_win->pixels_bytes_len;
	virtual_win->pixels = malloc(size);
	if (!virtual_win->pixels) {
		die("could not allocate %zu bytes?", size);
	}

	struct sdl_texture_buffer texture_buf;
	texture_buf.texture = NULL;
	texture_buf.pixel_buf = virtual_win;

	unsigned long escaped = 0;
	unsigned long not_escaped = 0;
	struct coordinate_plane_s *coord_plane =
	    new_coordinate_plane(window_x, window_y, cx_min, cx_max);
	print_directions(title, cx_min, cx_max);
	update_pixel_buffer(coord_plane, virtual_win, &escaped, &not_escaped);

	int x = SDL_WINDOWPOS_UNDEFINED;
	int y = SDL_WINDOWPOS_UNDEFINED;
	flags = SDL_WINDOW_RESIZABLE;
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

	struct timespec start;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	struct timespec last_print;
	last_print.tv_sec = start.tv_sec;
	last_print.tv_nsec = start.tv_nsec;

	unsigned long total_elapsed_seconds = 0;
	unsigned long frames_since_print = 0;
	unsigned long frame_count = 0;
	unsigned long iteration_count = 0;

	struct human_input input[2];
	init_input(&input[0]);
	init_input(&input[1]);
	struct human_input *tmp_input = NULL;
	struct human_input *new_input = &input[1];
	struct human_input *old_input = &input[0];

	int shutdown = 0;
	while (!shutdown) {
		tmp_input = new_input;
		new_input = old_input;
		old_input = tmp_input;
		init_input(new_input);

		while (SDL_PollEvent(&event)) {
			if ((shutdown =
			     sdl_process_event(&event_ctx, new_input))) {
				break;
			}
		}

		int press = 0;
		if (shutdown || (shutdown = process_input(new_input, &press))) {
			break;
		}
		if (event_ctx.resized || press) {
			SDL_GetWindowSize(window, &window_x, &window_y);
			delete_coordinate_plane(coord_plane);

			long double diff = cx_max - cx_min;
			switch (press) {
			case 'w':{
					long double zoom_diff = diff / 8;
					cx_min = cx_min + zoom_diff;
					cx_max = cx_max - zoom_diff;
					break;
				}
			case 's':{
					long double zoom_diff = diff / 8;
					cx_min = cx_min - zoom_diff;
					cx_max = cx_max + zoom_diff;
					break;
				}
			case 'a':{
					long double pan_diff = diff / 4;
					cx_min = cx_min - pan_diff;
					cx_max = cx_max - pan_diff;
					break;
				}
			case 'd':{
					long double pan_diff = diff / 4;
					cx_min = cx_min + pan_diff;
					cx_max = cx_max + pan_diff;
					break;
				}
			case ' ':{
					++func_idx;
					if (func_idx < 0 || func_idx > 4) {
						func_idx = 0;
					}
					title = nf[func_idx].name;
					SDL_SetWindowTitle(window, title);

				}
			default:
				/* reset */
				break;
			}

			coord_plane = new_coordinate_plane(window_x, window_y,
							   cx_min, cx_max);
			print_directions(title, cx_min, cx_max);
			event_ctx.resized = 0;
			iteration_count = 0;
		}

		iterate_plane(coord_plane, nf[func_idx].pfunc);
		update_pixel_buffer(coord_plane, virtual_win, &escaped,
				    &not_escaped);
		sdl_blit_texture(renderer, &texture_buf);

		iteration_count++;
		frame_count++;
		frames_since_print++;

		struct timespec now;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);

		struct timespec elapsed;
		diff_timespecs(last_print, now, &elapsed);

		if (((elapsed.tv_sec * 1000000000) + elapsed.tv_nsec) >
		    100000000) {
			double fps =
			    ((double)frames_since_print) /
			    (((double)elapsed.tv_sec) * 1000000000.0 +
			     ((double)elapsed.tv_nsec) / 1000000000.0);
			frames_since_print = 0;
			last_print.tv_sec = now.tv_sec;
			last_print.tv_nsec = now.tv_nsec;
			diff_timespecs(start, now, &elapsed);
			total_elapsed_seconds = elapsed.tv_sec + 1;
			int fps_printer = 1;	// make configurable?
			if (fps_printer) {
				fprintf(stdout,
					"iteation: %lu, escaped: %lu, not escaped: %lu fps: %.02f (avg fps: %.f)  \r",
					iteration_count, escaped, not_escaped,
					fps,
					(double)frame_count /
					(double)total_elapsed_seconds);
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
		delete_coordinate_plane(coord_plane);
		free(virtual_win->pixels);
		free(virtual_win);
	}

	return 0;
}
