default: sdl-coord-plane-iteration

CFLAGS += -g `sdl2-config --cflags` -Wextra -Wall -Wpedantic
LD_ADD += `sdl2-config --libs` -lm -lpthread

ifeq ($(findstring /usr/include/threads.h,$(wildcard /usr/include/*.h)),)
CFLAGS += '-DSKIP_THREADS'
endif

ifeq ($(findstring /usr/include/SDL2/SDL.h,$(wildcard /usr/include/SDL2/*.h)),)
CFLAGS += '-DSKIP_SDL'
endif

ifdef DEBUG
MAKEFILE_DEBUG=$(DEBUG)
else
MAKEFILE_DEBUG=0
endif
ifeq ($(MAKEFILE_DEBUG),0)
CFLAGS += '-DNDEBUG'
else
CFLAGS += '-DDEBUG'
endif

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0

sdl-coord-plane-iteration: sdl-coord-plane-iteration.c
	cc $(CFLAGS) ./sdl-coord-plane-iteration.c \
		-o ./sdl-coord-plane-iteration $(LD_ADD)

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
		./sdl-coord-plane-iteration.c



clean:
	rm -rf `cat .gitignore | sed -e 's/#.*//'`
