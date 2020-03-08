default: check

CC=cc

CFLAGS += -g -Wextra -Wall -Wpedantic -rdynamic -Isrc/
LDLIBS += -lm

ifeq ($(findstring /usr/include/threads.h,$(wildcard /usr/include/*.h)),)
CFLAGS += -DSKIP_THREADS
else
LDLIBS += -lpthread
endif

ifeq ($(findstring /usr/include/SDL2/SDL.h,$(wildcard /usr/include/SDL2/*.h)),)
BEST_CHECK=cli-demo
else
BEST_CHECK=sdl-demo
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
SHELL=/bin/bash

HEADERS=src/logerr-die.h \
	src/alloc-or-die.h \
	src/rgb-hsv.h \
	src/basic-thread-pool.h \
	src/coord-plane-option-parser.h \
	src/coord-plane-iteration.h \
	src/pixel-coord-plane-iteration.h

SOURCES=src/logerr-die.c \
	src/rgb-hsv.c \
	src/basic-thread-pool.c \
	src/coord-plane-option-parser.c \
	src/coord-plane-iteration.c

SDL_SOURCES=$(SOURCES) \
	src/pixel-coord-plane-iteration.c \
	src/sdl-coord-plane-iteration.c

SDL_HEADERS=$(HEADERS) \
	src/pixel-coord-plane-iteration.h


sdl-coord-plane-iteration: $(SDL_SOURCES) $(SDL_HEADERS)
	$(CC) $(CFLAGS) `sdl2-config --cflags` $(LDFLAGS) $(SDL_SOURCES) \
		-o ./sdl-coord-plane-iteration $(LDLIBS) `sdl2-config --libs`

sdl-demo: sdl-coord-plane-iteration
	./sdl-coord-plane-iteration

CLI_SOURCES=$(SOURCES) src/cli-coord-plane-iteration.c
CLI_HEADERS=$(HEADERS)

cli-coord-plane-iteration: $(CLI_SOURCES) $(CLI_HEADERS)
	$(CC) $(CFLAGS) -DNO_GUI=1 $(LDFLAGS) $(CLI_SOURCES) \
		-o ./cli-coord-plane-iteration $(LDLIBS)

cli-demo: cli-coord-plane-iteration
	./cli-coord-plane-iteration

check: cli-coord-plane-iteration $(BEST_CHECK)

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
		-T basic_thread_pool_s \
		-T thread_pool_todo_s -T thread_pool_loop_context_s \
		src/*.c src/*.h

clean:
	rm -rf `cat .gitignore | sed -e 's/#.*//'`
	pushd src && rm -rf `cat ../.gitignore | sed -e 's/#.*//'` && popd
