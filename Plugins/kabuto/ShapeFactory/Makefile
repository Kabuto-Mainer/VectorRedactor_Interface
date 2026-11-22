all:
	@echo [Building libkabuto_ShapeFactory.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I. -g -Wall -Wextra -o libkabuto_ShapeFactory.so src/ShapeFactory.cpp

clean:
	rm -rf *.so

.PHONY: all clean
