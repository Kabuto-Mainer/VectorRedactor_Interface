all:
	@echo [Building libDocument.so]
	clang++ -std=c++23 -fpic -shared -I../../../SDK -Iinclude -I. -g -Wall -Wextra -o libDocument.so src/Document.cpp

clean:
	rm -rf *.so *.o

