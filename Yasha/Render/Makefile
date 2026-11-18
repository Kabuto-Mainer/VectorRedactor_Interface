CXX := clang++
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null || printf '%s' '-lraylib')

all: libRender.so

libRender.so: src/Render.cpp include/Render.hpp ../../SDK/Interfaces/IRenderer.hpp
	$(CXX) -std=c++23 -fpic -shared -Iinclude -I../../SDK $(RAYLIB_CFLAGS) -g -Wall -Wextra -o $@ src/Render.cpp $(RAYLIB_LIBS)

clean:
	rm -f libRender.so

.PHONY: all clean
