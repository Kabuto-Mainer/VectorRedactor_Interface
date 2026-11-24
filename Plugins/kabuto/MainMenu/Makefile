all:
	@echo [Building libkabuto_MainMenu.so]
	clang++ -std=c++23 -fpic -shared -I../UIFW -Iinclude -I../../../SDK -I../../../ -I. -g -Wall -Wextra -o libkabuto_MainMenu.so src/MainMenu.cpp

clean:
	rm -rf *.so

.PHONY: all clean
