all:
	@echo [Building libkabuto_ToolManager.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I../../../ -I. -g -Wall -Wextra -o libkabuto_ToolManager.so src/ToolManager.cpp

clean:
	rm -rf *.so

.PHONY: all clean
