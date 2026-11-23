CXX = clang++
CXXFLAGS = -std=c++23 -g -Wall -Wextra
INCLUDES = -Iinclude -I../../../SDK
SRCS = ../../../SDK/Core/ApplicationCore.cpp src/Launcher.cpp
BASE_NAME = app

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    TARGET = $(BASE_NAME)
    LDFLAGS = -Wl,-export_dynamic
else
    TARGET = $(BASE_NAME)
    LDFLAGS = -rdynamic -ldl
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "Building $(TARGET) on $(UNAME_S)..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -rf $(BASE_NAME) $(BASE_NAME).dylib *.dSYM

.PHONY: all clean
