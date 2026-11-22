all:
	make -C ../UIFW
	@echo [Building libToolBar.so]
	clang++ -std=c++23 -fpic -shared -lUIFW -L../UIFW -Wl,-rpath='$$ORIGIN'/../UIFW -I../../../SDK -Iinclude -I.. -I. -g -Wall -Wextra -o libToolBar.so src/ToolBar.cpp

clean:
	rm -rf *.so *.o

