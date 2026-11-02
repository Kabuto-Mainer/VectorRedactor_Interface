all:
	@echo [Building GUIEngine.so]
	clang++ -std=c++23 -fpic -shared -I../plugins_usage/include -I../../SDK -I../.. -g -ldl -lSDL3 -lSDL3_gfx -Wall -Wextra -o GUIEngine.so ../plugins_usage/src/GUIEngine.cpp
	@echo [Building UIWi.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK/cheets/UIWi/include -I../../SDK -g -Wall -Wextra -o UIWi.so ../../SDK/cheets/UIWi/src/UIWi.cpp
	@echo [Building MainPlugin.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -I../../SDK/cheets/UIWi/include -g -Wall -Wextra -o MainPlugin.so src/MainPlugin.cpp
	@echo [Building main]
	clang++ -std=c++23 -rdynamic -Iinclude -I../../SDK -g -ldl -Wall -Wextra -o main src/Main.cpp ../../SDK/Core/ApplicationCore.cpp

clean:
	rm -rf *.so main

.PHONY: all clean
