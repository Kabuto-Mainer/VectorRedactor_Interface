all:
	@echo [Building libkabuto_Document.so]
	clang++ -std=c++23 -fpic -shared -I../../../SDK -I. -g -Wall -Wextra -o libkabuto_Document.so src/Document.cpp

clean:
	rm -rf *.so

.PHONY: all clean
