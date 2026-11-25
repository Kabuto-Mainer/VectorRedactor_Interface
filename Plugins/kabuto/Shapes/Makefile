CXX      := clang++
CXXFLAGS ?= -std=c++23 -fpic -g -Wall -Wextra
INCLUDES := -I../../../SDK -I../../../ -I. -Iinclude

SOURCES := src/Polygon.cpp src/PolygonComponent.cpp
HEADERS := $(wildcard export/*.hpp)
TARGET  := libkabuto_Shapes.so
LIBTESS := libs/libtess2.a

all: $(TARGET)

$(LIBTESS):
	$(MAKE) -C libs

$(TARGET): $(SOURCES) $(HEADERS) $(LIBTESS)
	@echo [Building $(TARGET)]
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared -o $@ $(SOURCES) $(LIBTESS)

clean:
	$(MAKE) -C libs clean
	rm -f $(TARGET)

.PHONY: all clean
