all:
	@echo [Building libDesktop.so]
	clang++ -std=c++23 -fpic -shared -I../../../SDK -Iinclude -I. -g -Wall -Wextra -o libDesktop.so src/Desktop.cpp

clean:
	rm -rf *.so *.o

