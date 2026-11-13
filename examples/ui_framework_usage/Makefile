all:
	make -C ../../Plugins/SDL_GUIEngine
	make -C ../../Plugins/MainPlugin
	make -C ../../Plugins/UIFW
	make -C ../../Plugins/Launcher APPLICATION_EXECUTABLE_NAME=../../examples/ui_framework_usage/app
	@echo [Building libMainPlugin.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libMainPlugin.so src/MainPlugin.cpp

clean:
	rm -rf *.so app

.PHONY: all clean

