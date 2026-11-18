CXX := clang++
CPPFLAGS := -I../../SDK -I. -IToolBar/include -IDesktop/include -IDocument/include -IToolManager/include
CXXFLAGS := -std=c++23 -g -Wall -Wextra -MMD -MP
LDFLAGS := -rdynamic
LDLIBS := -ldl

SOURCES := Launcher.cpp Desktop/src/Desktop.cpp Document/src/Document.cpp UIFramework/Button.cpp UIFramework/ButtonPool.cpp \
           UIFramework/ColorButton.cpp UIFramework/IconButton.cpp
OBJECTS := $(addprefix build/,$(SOURCES:.cpp=.o)) build/Core/ApplicationCore.o
DEPS := $(OBJECTS:.o=.d)

all: Render/libRender.so InputPump/libInputPump.so ToolManager/ToolManager.so ToolBar/libToolBar.so RectangleTool/libRectangleTool.so EllipseTool/libEllipseTool.so SelectorTool/libSelectorTool.so app

Render/libRender.so: Render/Makefile Render/src/Render.cpp Render/include/Render.hpp ../../SDK/Interfaces/IRenderer.hpp
	$(MAKE) -C Render

InputPump/libInputPump.so: InputPump/Makefile InputPump/src/InputPump.cpp InputPump/include/InputPump.hpp ../../SDK/Interfaces/IInputPump.hpp
	$(MAKE) -C InputPump

ToolManager/ToolManager.so: ToolManager/Makefile ToolManager/src/ToolManager.cpp ToolManager/include/ToolManager.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C ToolManager

ToolBar/libToolBar.so: ToolBar/Makefile ToolBar/src/Plugin.cpp ToolBar/src/ToolBar.cpp ToolBar/include/ToolBar.hpp ToolBar/include/ToolBarIID.hpp ToolManager/include/ToolManager.hpp
	$(MAKE) -C ToolBar

RectangleTool/libRectangleTool.so: RectangleTool/Makefile RectangleTool/src/Plugin.cpp RectangleTool/src/RectangleTool.cpp RectangleTool/src/RectangleShape.cpp RectangleTool/include/RectangleTool.hpp RectangleTool/include/RectangleShape.hpp ../../SDK/Interfaces/IDocument.hpp ../../SDK/Interfaces/IToolBar.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C RectangleTool

EllipseTool/libEllipseTool.so: EllipseTool/Makefile EllipseTool/src/Plugin.cpp EllipseTool/src/EllipseTool.cpp EllipseTool/src/EllipseShape.cpp EllipseTool/include/EllipseTool.hpp EllipseTool/include/EllipseShape.hpp ../../SDK/Interfaces/IDocument.hpp ../../SDK/Interfaces/IToolBar.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C EllipseTool

SelectorTool/libSelectorTool.so: SelectorTool/Makefile SelectorTool/src/Plugin.cpp SelectorTool/src/SelectorTool.cpp SelectorTool/src/SelectionOverlay.cpp SelectorTool/src/ShapeLookup.cpp SelectorTool/include/SelectorTool.hpp SelectorTool/include/SelectionOverlay.hpp SelectorTool/include/ShapeLookup.hpp ToolManager/include/ToolManager.hpp ../../SDK/Interfaces/IDocument.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C SelectorTool

app: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build/Core/ApplicationCore.o: ../../SDK/Core/ApplicationCore.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build app ToolManager/ToolManager.so Render/libRender.so InputPump/libInputPump.so
	$(MAKE) -C RectangleTool clean
	$(MAKE) -C EllipseTool clean
	$(MAKE) -C SelectorTool clean
	$(MAKE) -C ToolBar clean

-include $(DEPS)

.PHONY: all clean
