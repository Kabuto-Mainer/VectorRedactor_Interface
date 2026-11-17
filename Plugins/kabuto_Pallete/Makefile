all:
	@echo [Building libkabuto_Pallete.so]
	clang++ -std=c++23 -fpic -shared -I../kabuto_UIFW -Iinclude -I../../SDK -I. -g -Wall -Wextra -o libkabuto_Pallete.so src/Palette.cpp

clean:
	rm -rf *.so

.PHONY: all clean
