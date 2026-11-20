all:
	@echo [Building libkabuto_Serialize.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I../../../ -I. -g -Wall -Wextra -o libkabuto_Serialize.so src/Serialize.cpp

clean:
	rm -rf *.so

.PHONY: all clean
