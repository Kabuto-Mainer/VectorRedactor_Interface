all: app

.PHONY: plugins
plugins:
	make -C ../../Plugins/kabuto/SDL3Engine
	make -C ../../Plugins/kabuto/UIFW
	make -C ../../Plugins/kabuto/Desktop
	make -C ../../Plugins/kabuto/Document
	make -C ../../Plugins/kabuto/Palette
	make -C ../../Plugins/murugov/ToolManager
	make -C ../../Plugins/kabuto/Tools

app: plugins
	@echo [Building ToolsPrototype]
	clang++ -std=c++23 -rdynamic -g -ldl -Wall -Wextra -I../../SDK -I../../Plugins/kabuto/UIFW src/main.cpp ../../SDK/Core/ApplicationCore.cpp -o app

run: app
	./app

clean:
	rm -rf app

.PHONY: all clean run
