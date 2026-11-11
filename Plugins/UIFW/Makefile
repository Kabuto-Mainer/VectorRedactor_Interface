all:
	@echo [Building libUIFW.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -I. -g -Wall -Wextra -o libUIFW.so src/UIFW.cpp

clean:
	rm -rf *.so

.PHONY: all clean
