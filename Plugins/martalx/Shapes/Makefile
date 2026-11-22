all:
	@echo [Building libRectangleShape.so]
	clang++ -std=c++23 -fpic -shared -I../../../SDK -Iinclude -I.. -I. -g -Wall -Wextra -o libRectangleShape.so src/RectangleShape.cpp

clean:
	rm -rf *.so *.o

