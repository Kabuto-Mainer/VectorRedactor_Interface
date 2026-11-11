APPLICATION_EXECUTABLE_NAME ?= app

all:
	@echo [Building $(APPLICATION_EXECUTABLE_NAME)]
	clang++ -std=c++23 -rdynamic -g -ldl -Wall -Wextra -I../../SDK src/Launcher.cpp -Iinclude -I. ../../SDK/Core/ApplicationCore.cpp -o $(APPLICATION_EXECUTABLE_NAME)

clean:
	rm -rf $(APPLICATION_EXECUTABLE_NAME)

.PHONY: all clean

