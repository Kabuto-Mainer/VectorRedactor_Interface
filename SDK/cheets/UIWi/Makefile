all:
	@echo [Building UIWi.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -g -Wall -Wextra -o UIWi.so src/UIWi.cpp

clean:
	rm -rf *.so

.PHONY: all clean
