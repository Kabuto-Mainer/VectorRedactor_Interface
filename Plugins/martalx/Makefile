
all:
	make -C ../Launcher APPLICATION_EXECUTABLE_NAME=../martalx/app
	@echo [Building libSDL_GUIEngine.so]
	clang++ -std=c++23 -fpic -shared -lSDL3 -lSDL3_gfx -lSDL3_image -lSDL3_ttf -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libSDL_GUIEngine.so src/SDL_GUIEngine.cpp
	@echo [Building libDesktop.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libDesktop.so src/Desktop.cpp
	@echo [Building libMainPlugin.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libMainPlugin.so src/MainPlugin.cpp src/UIFramework.cpp

clean:
	rm -rf *.so app

.PHONY: all clean

