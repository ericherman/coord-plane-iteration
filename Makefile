default: all

CC=cc

CFLAGS += -g -Wextra -Wall -Wpedantic -rdynamic -Isrc/
LDLIBS += -lm

ifeq ($(findstring /usr/include/threads.h,$(wildcard /usr/include/*.h)),)
CFLAGS += -DSKIP_THREADS
else
LDLIBS += -lpthread
endif

BUILD_CFLAGS += -DNDEBUG -O2
DEBUG_CFLAGS += -DDEBUG -O0 -DMake_valgrind_happy=1

ifeq ($(findstring /usr/include/SDL2/SDL.h,$(wildcard /usr/include/SDL2/*.h)),)
BEST_DEMO=build/cli-coord-plane-iteration
else
BEST_DEMO=build/sdl-coord-plane-iteration
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

CLI_SOURCES=$(SOURCES) src/cli-coord-plane-iteration.c
CLI_HEADERS=$(HEADERS)

build/sdl-coord-plane-iteration: $(SDL_SOURCES) $(SDL_HEADERS)
	mkdir -pv build
	$(CC) $(BUILD_CFLAGS) $(CFLAGS) `sdl2-config --cflags` $(LDFLAGS) \
		$(SDL_SOURCES) -o $@ $(LDLIBS) `sdl2-config --libs`

debug/sdl-coord-plane-iteration: $(SDL_SOURCES) $(SDL_HEADERS)
	mkdir -pv debug
	$(CC) $(DEBUG_CFLAGS) $(CFLAGS) `sdl2-config --cflags` $(LDFLAGS) \
		$(SDL_SOURCES) -o $@ $(LDLIBS) `sdl2-config --libs`

build/cli-coord-plane-iteration: $(CLI_SOURCES) $(CLI_HEADERS)
	mkdir -pv build
	$(CC) $(BUILD_CFLAGS) $(CFLAGS) -DNO_GUI=1 $(LDFLAGS) \
		$(CLI_SOURCES) -o $@ $(LDLIBS)

debug/cli-coord-plane-iteration: $(CLI_SOURCES) $(CLI_HEADERS)
	mkdir -pv debug
	$(CC) $(DEBUG_CFLAGS) $(CFLAGS) -DNO_GUI=1 $(LDFLAGS) \
		$(CLI_SOURCES) -o $@ $(LDLIBS)

all-sdl: build/sdl-coord-plane-iteration debug/sdl-coord-plane-iteration

all-cli: build/cli-coord-plane-iteration debug/cli-coord-plane-iteration

all-build: build/cli-coord-plane-iteration build/sdl-coord-plane-iteration

all-debug: debug/cli-coord-plane-iteration debug/sdl-coord-plane-iteration

all: all-build all-debug

sdl-demo: build/sdl-coord-plane-iteration
	$<

cli-demo: build/cli-coord-plane-iteration
	$<

demo: $(BEST_DEMO)
	$(BEST_DEMO)

build/check.out: build/cli-coord-plane-iteration
	build/cli-coord-plane-iteration --height=24 --width=79 \
		--halt_after=1000 \
		| tail -n1 > build/check.out

check: build/check.out $(BEST_DEMO)
	$(BEST_DEMO) --halt_after=1000
	grep "escaped: 1642 not: 254" build/check.out
	@echo SUCCESS $@

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
		-T coordinate_plane_s \
		-T coordinate_plane_iterate_context_s \
		-T coord_options_s \
		-T basic_thread_pool_s \
		-T basic_thread_pool_todo_s \
		-T basic_thread_pool_loop_context_s \
		src/*.c src/*.h

clean:
	rm -rf `cat .gitignore | sed -e 's/#.*//'`
	pushd src && rm -rf `cat ../.gitignore | sed -e 's/#.*//'` && popd
