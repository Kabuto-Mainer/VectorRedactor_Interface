all:
	@echo [Building libkabuto_Tools.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../../SDK -I../../../ -I../Shapes -I../Palette -I../UIFW -I../Document -I. -g -Wall -Wextra -o libkabuto_Tools.so src/Tools.cpp -L../Shapes -l:libkabuto_Shapes.so -Wl,-rpath,'$$ORIGIN/../Shapes'

clean:
	rm -rf *.so

.PHONY: all clean
