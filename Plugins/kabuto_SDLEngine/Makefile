all:
	@echo [Building SDL3Engine.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -g -ldl -lSDL3 -lSDL3_gfx -lSDL3_ttf -lSDL3_image -Wall -Wextra -o SDL3Engine.so src/SDL3Engine.cpp

clean:
	rm -rf *.so

.PHONY: all clean
