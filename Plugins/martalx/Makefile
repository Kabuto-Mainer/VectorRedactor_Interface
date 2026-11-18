all:
	make -C ../../Plugins/SDL_GUIEngine
	make -C ../../Plugins/Launcher APPLICATION_EXECUTABLE_NAME=../../examples/minimal_redactor/app
	@echo [Building libDesktop.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libDesktop.so src/Desktop.cpp
	@echo [Building libMainPlugin.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libMainPlugin.so src/MainPlugin.cpp src/UIFramework.cpp

clean:
	rm -rf *.so app

.PHONY: all clean

