all:
	@echo [Building libkabuto_ToolBar.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I../UIFW -I. -g -Wall -Wextra -o libkabuto_ToolBar.so src/ToolBar.cpp

clean:
	rm -rf *.so

.PHONY: all clean
