all:
	@echo [Building SDL_GUIEngine.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -ldl -lSDL3 -lSDL3_gfx -Wall -Wextra -o libSDL_GUIEngine.so src/SDL_GUIEngine.cpp

clean:
	rm -rf *.so

.PHONY: all clean
