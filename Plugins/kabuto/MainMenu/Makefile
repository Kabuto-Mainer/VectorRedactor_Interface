CXX      := clang++
CXXFLAGS := -std=c++23 -fpic -g -Wall -Wextra
INCLUDES := -I../../../SDK -I../../../Plugins/ -I.
LDLIBS   := -ldl -ljsoncpp

SRC     := src/MainMenu.cpp
HEADERS := $(wildcard export/*.hpp) $(wildcard include/*.hpp)
LIB     := libkabuto_MainMenu.so

all: $(LIB)

$(LIB): $(SRC) $(HEADERS)
	@echo [Building $(LIB)]
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared -o $@ $(SRC) $(LDLIBS)

clean:
	rm -f $(LIB)

.PHONY: all clean
