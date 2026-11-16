all:
	make -C ../../Plugins/SDL_GUIEngine
	make -C ../../Plugins/UIFW
	make -C ../../Plugins/MainPlugin
	make -C ../../Plugins/Editor
	make -C ../../Plugins/Launcher APPLICATION_EXECUTABLE_NAME=../../Apps/MiniEditor/app
	@echo [Building libMiniEditor.so]
	clang++ -std=c++23 -fpic -shared -Iinclude -I../../SDK -I../../Plugins/VDocument -I../../Plugins/UIFW -I. -g -Wall -Wextra -o libMiniEditor.so src/MiniEditor.cpp

clean:
	rm -rf *.so app

.PHONY: all clean
