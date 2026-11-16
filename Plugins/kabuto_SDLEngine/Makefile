all:
	@echo [Building SDL3Engine.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../SDK -g -ldl -lSDL3 -lSDL3_gfx -lSDL3_ttf -lSDL3_image -Wall -Wextra -o SDL3Engine.so src/SDL3Engine.cpp src/SDL3Renderer.cpp
	@echo [Building main]
	clang++ -std=c++23 -rdynamic -Iinclude -I../SDK -g -ldl -Wall -Wextra -o main src/Main.cpp ../SDK/Core/ApplicationCore.cpp

clean:
	rm -rf *.so main

.PHONY: all clean
1
