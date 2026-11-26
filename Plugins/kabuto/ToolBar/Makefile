CXX      := clang++
CXXFLAGS := -std=c++23 -fpic -g -Wall -Wextra
INCLUDES := -I../../../SDK -I../../.. -I../../../Plugins/ -I. -Iinclude
LDLIBS   := -ldl -ljsoncpp

SRC     := src/ToolBar.cpp
HEADERS := $(wildcard export/*.hpp) $(wildcard include/*.hpp)
LIB     := libkabuto_ToolBar.so

all: $(LIB)

$(LIB): $(SRC) $(HEADERS)
	@echo [Building $(LIB)]
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared -o $@ $(SRC) $(LDLIBS)

clean:
	rm -f $(LIB)

.PHONY: all clean
