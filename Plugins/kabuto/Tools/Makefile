all:
	@echo [Building libkabuto_Tools.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I../Document -I../Palette -I../UIFW -I. -g -Wall -Wextra -o libkabuto_Tools.so src/Tools.cpp

clean:
	rm -rf *.so

.PHONY: all clean
