all:
	@echo [Building libMainPlugin.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -I. -g -Wall -Wextra -o libMainPlugin.so src/MainPlugin.cpp

clean:
	rm -rf *.so

.PHONY: all clean
