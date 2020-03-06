default: sdl-coord-plane-iteration

CFLAGS += -g -Wextra -Wall -Wpedantic -rdynamic
LDLIBS += -lm

ifeq ($(findstring /usr/include/threads.h,$(wildcard /usr/include/*.h)),)
CFLAGS += -DSKIP_THREADS
else
LDLIBS += -lpthread
endif

ifeq ($(findstring /usr/include/SDL2/SDL.h,$(wildcard /usr/include/SDL2/*.h)),)
CFLAGS += -DSKIP_SDL
else
CFLAGS += `sdl2-config --cflags`
LDLIBS += `sdl2-config --libs`
endif

ifdef DEBUG
MAKEFILE_DEBUG=$(DEBUG)
else
MAKEFILE_DEBUG=0
endif
ifeq ($(MAKEFILE_DEBUG),0)
CFLAGS += -DNDEBUG -O2
else
CFLAGS += -DDEBUG -O0
endif

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0

sdl-coord-plane-iteration: sdl-coord-plane-iteration.c
	cc $(CFLAGS) $(LDFLAGS) ./sdl-coord-plane-iteration.c \
		-o ./sdl-coord-plane-iteration $(LDLIBS)

check: sdl-coord-plane-iteration
	./sdl-coord-plane-iteration

tidy:
	$(LINDENT) \
		-T FILE -T size_t -T ssize_t -T bool \
		-T int8_t -T int16_t -T int32_t -T int64_t \
		-T uint8_t -T uint16_t -T uint32_t -T uint64_t \
		-T SDL_Window -T SDL_Renderer -T SDL_Event -T SDL_Texture \
		-T sdl_event_context_s -T sdl_texture_buffer_s \
		-T pixel_buffer_s -T keyboard_key_s -T human_input_s \
		-T hsv_s -T rgb_s -T rgb24_s \
		-T ldxy_s -T iterxy_s \
		-T named_pfunc_s -T pfunc_f \
		-T coordinate_plane_s -T coordinate_plane_iterate_context_s \
		-T coord_options_s \
		-T thread_pool_s \
		-T thread_pool_todo_s -T thread_pool_loop_context_s \
		./sdl-coord-plane-iteration.c



clean:
	rm -rf `cat .gitignore | sed -e 's/#.*//'`
