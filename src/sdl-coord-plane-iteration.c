/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* sdl-coord-plane-iteration.c: playing with mandlebrot and such */
/* Copyright (C) 2020-2023 Eric Herman <eric@freesa.org> */
/* https://github.com/ericherman/coord-plane-iteration */
/*
   cc -DNDEBUG -O2 -g -Wextra -Wall -Wpedantic -rdynamic -Isrc/ \
        `sdl2-config --cflags` \
	src/logerr-die.c src/alloc-or-die.c src/rgb-hsv.c \
	src/basic-thread-pool.c src/coord-plane-option-parser.c \
	src/coord-plane-iteration.c src/pixel-coord-plane-iteration.c \
	src/sdl-coord-plane-iteration.c \
	-o build/sdl-coord-plane-iteration \
	-lm -lpthread `sdl2-config --libs` && \
   ./sdl-coord-plane-iteration
*/

#define SDL_COORD_PLANE_ITERATION_VERSION "0.2.0"

#include <signal.h>
#include <sys/time.h>
#include <SDL.h>

#include "alloc-or-die.h"
#include "coord-plane-option-parser.h"
#include "pixel-coord-plane-iteration.h"

#ifndef Make_valgrind_happy
#define Make_valgrind_happy 0
#endif

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
	int window_x = coordinate_plane_win_width(plane);
	int window_y = coordinate_plane_win_height(plane);

	Uint32 init_flags = SDL_INIT_VIDEO;
	if (SDL_Init(init_flags) != 0) {
		die("Could not SDL_Init(%lu) (%s)", (uint64_t)init_flags,
		    SDL_GetError());
	}

	sdl_texture_buffer_s texture_buf;
	texture_buf.texture = NULL;
	texture_buf.pixel_buf = virtual_win;

	print_directions(plane, stdout);
	fflush(stdout);

	int x = SDL_WINDOWPOS_UNDEFINED;
	int y = SDL_WINDOWPOS_UNDEFINED;
	Uint32 win_flags = SDL_WINDOW_RESIZABLE;
	const char *title = coordinate_plane_function_name(plane);
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
			if (sdl_process_event(&event_ctx, new_input)) {
				shutdown = 1;
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
			bool preseve_ratio = false;
			coordinate_plane_resize(plane, window_x, window_y,
						preseve_ratio);
			change = coordinate_plane_change_yes;
			event_ctx.resized = false;
		}
		if (change == coordinate_plane_change_yes) {
			iterations_at_last_print = 0;
			title = coordinate_plane_function_name(plane);
			SDL_SetWindowTitle(window, title);
			print_directions(plane, stdout);
			fflush(stdout);
		}
		// set a 32 frames per second target
		uint64_t usec_per_frame_high_threshold = usec_per_sec / 30;
		uint64_t usec_per_frame_low_threshold = usec_per_sec / 45;
		uint64_t before = time_in_usec();

		coordinate_plane_iterate(plane, it_per_frame);
		uint64_t it_count = coordinate_plane_iteration_count(plane);
		if (coordinate_plane_halt_after(plane) &&
		    (it_count >= coordinate_plane_halt_after(plane))) {
			shutdown = 1;
		}
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
			if (it_per_frame < 10) {
				--it_per_frame;
			} else {
				double ht = 1.0 * usec_per_frame_high_threshold;
				double ratio = ht / diff;
				uint32_t new_per_frame = it_per_frame * ratio;
				if (new_per_frame >= it_per_frame) {
					--it_per_frame;
				} else {
					it_per_frame = new_per_frame;
				}
				if (it_per_frame == 0) {
					it_per_frame = 1;
				}
			}
		}

		uint64_t elapsed_since_last_print = now - last_print;
		if (shutdown || elapsed_since_last_print > usec_per_sec) {
			double seconds_elapsed_since_last_print =
			    (1.0 * elapsed_since_last_print) / usec_per_sec;
			double fps =
			    frames_since_print /
			    seconds_elapsed_since_last_print;
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
				size_t escaped =
				    coordinate_plane_escaped_count(plane);
				size_t not_escaped =
				    coordinate_plane_not_escaped_count(plane);
				size_t num_threads =
				    coordinate_plane_num_threads(plane);
				fprintf(stdout,
					"i:%" PRIu64 " escaped: %" PRIu64
					" not: %" PRIu64
					" (ips: %.f fps: %.f ipf: %" PRIu32
					" thds: %zu)     \r", it_count, escaped,
					not_escaped, ips, fps, it_per_frame,
					num_threads);
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
	signal(SIGSEGV, backtrace_exit_handler);

	const char *version = SDL_COORD_PLANE_ITERATION_VERSION;
	coordinate_plane_s *plane =
	    coordinate_plane_new_from_args(argc, argv, version);

	size_t palette_len = 1024;
	pixel_buffer_s *virtual_win =
	    pixel_buffer_new_from_plane(plane, palette_len);

	sdl_coord_plane_iteration(plane, virtual_win);

	if (Make_valgrind_happy) {
		pixel_buffer_free(virtual_win);
		coordinate_plane_free(plane);
	}
}
