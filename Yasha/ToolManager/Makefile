all:
	@echo [Building ToolManager.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -I. -g -Wall -Wextra -o ToolManager.so src/ToolManager.cpp

clean:
	rm -rf *.so

.PHONY: all clean