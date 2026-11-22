all:
	@echo [Building libMainPlugin.so]
	clang++ -std=c++23 -fpic -shared -I../../../SDK -Iinclude -I.. -I. -g -Wall -Wextra -o libMainPlugin.so src/MainPlugin.cpp

clean:
	rm -rf *.so *.o

