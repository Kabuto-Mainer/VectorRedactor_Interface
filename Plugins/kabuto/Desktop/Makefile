all:
	@echo [Building libkabuto_Desktop.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I. -g -Wall -Wextra -o libkabuto_Desktop.so src/Desktop.cpp

clean:
	rm -rf *.so

.PHONY: all clean
