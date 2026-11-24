all:
	@echo [Building libkabuto_UIFW.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I. -g -Wall -Wextra -o libkabuto_UIFW.so src/UIFW.cpp

clean:
	rm -rf *.so

.PHONY: all clean
