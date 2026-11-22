all:
	@echo [Building libSDL_GUIEngine.so]
	clang++ -std=c++23 -fpic -shared -lSDL3 -lSDL3_gfx -lSDL3_image -lSDL3_ttf -I../../../SDK -Iinclude -I.. -I. -g -Wall -Wextra -o libSDL_GUIEngine.so src/SDL_GUIEngine.cpp

clean:
	rm -rf *.so *.o

