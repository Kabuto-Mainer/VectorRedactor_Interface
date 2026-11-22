all:
	@echo [Building libToolManager.so]
	clang++ -std=c++23 -fpic -shared -I../../../SDK -Iinclude -I. -g -Wall -Wextra -o libToolManager.so src/ToolManager.cpp

clean:
	rm -rf *.so *.o

