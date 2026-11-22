all:
	make -C ../UIFW
	make -C ../Shapes
	@echo [Building libRectangleTool.so]
	clang++ -std=c++23 -fpic -shared -lUIFW -lRectangleShape -L../UIFW -L../Shapes -Wl,-rpath='$$ORIGIN'/../UIFW,-rpath='$$ORIGIN'/../Shapes -I../../../SDK -Iinclude -I.. -I. -g -Wall -Wextra -o libRectangleTool.so src/RectangleTool.cpp
	@echo [Building libBrowseTool.so]
	clang++ -std=c++23 -fpic -shared -lUIFW -lRectangleShape -L../UIFW -L../Shapes -Wl,-rpath='$$ORIGIN'/../UIFW,-rpath='$$ORIGIN'/../Shapes -I../../../SDK -Iinclude -I.. -I. -g -Wall -Wextra -o libBrowseTool.so src/BrowseTool.cpp

clean:
	rm -rf *.so *.o

