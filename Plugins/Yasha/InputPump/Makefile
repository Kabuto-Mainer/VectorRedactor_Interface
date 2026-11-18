CXX := clang++
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null || printf '%s' '-lraylib')

all: libInputPump.so

libInputPump.so: src/InputPump.cpp include/InputPump.hpp ../../../SDK/Interfaces/IInputPump.hpp
	$(CXX) -std=c++23 -fpic -shared -Iinclude -I../../../SDK $(RAYLIB_CFLAGS) -g -Wall -Wextra -o $@ src/InputPump.cpp $(RAYLIB_LIBS)

clean:
	rm -f libInputPump.so

.PHONY: all clean
