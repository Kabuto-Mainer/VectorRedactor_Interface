all:
	@echo [Building MainPlugin.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -ldl -g -Wall -Wextra -o MainPlugin.so src/MainPlugin.cpp
	@echo [Building GUIEngine.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -g -ldl -lSDL3 -lSDL3_gfx -Wall -Wextra -o GUIEngine.so src/GUIEngine.cpp
	@echo [Building main]
	clang++ -std=c++23 -rdynamic -Iinclude -I../../SDK -g -ldl -Wall -Wextra -o main src/Main.cpp ../../SDK/Core/ApplicationCore.cpp

clean:
	rm -rf *.so main

.PHONY: all clean
