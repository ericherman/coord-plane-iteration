default: sdl-coord-plane-iteration

CFLAGS=-g -O2 `sdl2-config --cflags` -Wextra -Wall -Wpedantic
LD_ADD=`sdl2-config --libs` -lm

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0

sdl-coord-plane-iteration: sdl-coord-plane-iteration.c
	cc $(CFLAGS) ./sdl-coord-plane-iteration.c \
		-o ./sdl-coord-plane-iteration $(LD_ADD)

check: sdl-coord-plane-iteration
	./sdl-coord-plane-iteration

tidy:
	$(LINDENT) \
		-T FILE \
		-T size_t -T ssize_t \
		-T SDL_Texture \
		-T SDL_Window \
		-T SDL_Renderer \
		-T SDL_Event \
		-T iterxy_s \
		sdl-coord-plane-iteration.c


clean:
	rm -rf `cat .gitignore | sed -e 's/#.*//'`
