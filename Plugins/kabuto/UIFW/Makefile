CXX      := clang++
CXXFLAGS := -std=c++23 -fpic -g -Wall -Wextra
INCLUDES := -I../../../SDK -I../../../Plugins/ -I.
LDLIBS   := -ldl -lSDL3 -lSDL3_gfx -lSDL3_ttf -lSDL3_image -ljsoncpp

SRC     := src/UIFW.cpp
HEADERS := $(wildcard export/*.hpp) $(wildcard include/*.hpp)
LIB     := libkabuto_UIFW.so

all: $(LIB)

$(LIB): $(SRC) $(HEADERS)
	@echo [Building $(LIB)]
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared -o $@ $(SRC) $(LDLIBS)

clean:
	rm -f $(LIB)

.PHONY: all clean
