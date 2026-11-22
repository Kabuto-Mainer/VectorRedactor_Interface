all:
	@echo [Building libUIFW.so]
	clang++ -std=c++23 -fpic -shared -I../../../SDK -Iinclude -I. -g -Wall -Wextra -o libUIFW.so src/UIFW.cpp

clean:
	rm -rf *.o *.so

