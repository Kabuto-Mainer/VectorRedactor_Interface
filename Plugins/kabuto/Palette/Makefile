all:
	@echo [Building libkabuto_Palette.so]
	clang++ -std=c++23 -fpic -shared -I../UIFW -Iinclude -I../../../SDK -I. -g -Wall -Wextra -o libkabuto_Palette.so src/Palette.cpp

clean:
	rm -rf *.so

.PHONY: all clean
