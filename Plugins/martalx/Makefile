
#./libSDL_GUIEngine.so
#./libDesktop.so
#./libDocument.so
#./libToolManager.so
#./libToolBar.so
#./libMainPlugin.so

all:
	make -C ../Launcher APPLICATION_EXECUTABLE_NAME=../martalx/app
	@echo [Building UIFramework.o]
	clang++ -std=c++23 -fpic -c -I../../SDK -Iinclude -I. -g -Wall -Wextra src/UIFramework.cpp
	@echo [Building Shapes.o]
	clang++ -std=c++23 -fpic -c -I../../SDK -Iinclude -I. -g -Wall -Wextra src/Shapes.cpp
	@echo [Building libSDL_GUIEngine.so]
	clang++ -std=c++23 -fpic -shared -lSDL3 -lSDL3_gfx -lSDL3_image -lSDL3_ttf -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libSDL_GUIEngine.so src/SDL_GUIEngine.cpp
	@echo [Building libDesktop.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libDesktop.so src/Desktop.cpp UIFramework.o
	@echo [Building libDocument.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libDocument.so src/Document.cpp UIFramework.o
	@echo [Building libToolManager.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libToolManager.so src/ToolManager.cpp UIFramework.o
	@echo [Building libToolBar.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libToolBar.so src/ToolBar.cpp UIFramework.o
	@echo [Building libTools.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libTools.so src/Tools.cpp UIFramework.o Shapes.o
	@echo [Building libMainPlugin.so]
	clang++ -std=c++23 -fpic -shared -I../../SDK -Iinclude -I. -g -Wall -Wextra -o libMainPlugin.so src/MainPlugin.cpp UIFramework.o

clean:
	rm -rf *.so app

.PHONY: all clean

