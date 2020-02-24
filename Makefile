default: sdl-coord-plane-iteration

CFLAGS=-g -O2 `sdl2-config --cflags`
LD_ADD=`sdl2-config --libs` -lm

sdl-coord-plane-iteration: sdl-coord-plane-iteration.c
	cc $(CFLAGS) ./sdl-coord-plane-iteration.c \
		-o ./sdl-coord-plane-iteration $(LD_ADD)

check: sdl-coord-plane-iteration
	./sdl-coord-plane-iteration

clean:
	rm -rf `cat .gitignore | sed -e 's/#.*//'`
