

.PHONY: plugins app

app:
	@./build_plugins.sh
	@echo [Building ToolsPrototype]
	clang++ -std=c++23 -rdynamic -g -ldl -Wall -Wextra -I../../SDK -I../../Plugins/kabuto/UIFW src/main.cpp ../../SDK/Core/ApplicationCore.cpp -o app

run: app
	./app

clean:
	rm -rf app

.PHONY: all clean run
