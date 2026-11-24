CXX := clang++
CPPFLAGS := -I../../SDK -I. -IToolBar/include -IDesktop/include -IDocument/include -IPalette/include -IToolManager/include
CXXFLAGS := -std=c++23 -g -Wall -Wextra -MMD -MP
LDFLAGS := -rdynamic
LDLIBS := -ldl -ljsoncpp

SOURCES := Launcher.cpp Desktop/src/Desktop.cpp Document/src/Document.cpp
OBJECTS := $(addprefix build/,$(SOURCES:.cpp=.o)) build/Core/ApplicationCore.o
DEPS := $(OBJECTS:.o=.d)

all: Render/libRender.so InputPump/libInputPump.so UIFramework/libUIFramework.so Palette/libPalette.so ToolManager/ToolManager.so ToolBar/libToolBar.so RectangleTool/libRectangleTool.so EllipseTool/libEllipseTool.so MurugovSelectTool/libMurugovSelectTool.so app

Render/libRender.so: Render/Makefile Render/src/Render.cpp Render/include/Render.hpp ../../SDK/Interfaces/IRenderer.hpp
	$(MAKE) -C Render

InputPump/libInputPump.so: InputPump/Makefile InputPump/src/InputPump.cpp InputPump/include/InputPump.hpp ../../SDK/Interfaces/IInputPump.hpp
	$(MAKE) -C InputPump

UIFramework/libUIFramework.so: UIFramework/Makefile UIFramework/Plugin.cpp UIFramework/Button.cpp UIFramework/ButtonPool.cpp UIFramework/ColorButton.cpp UIFramework/IconButton.cpp UIFramework/UIFramework.hpp
	$(MAKE) -C UIFramework

Palette/libPalette.so: Palette/Makefile Palette/src/Plugin.cpp Palette/src/Palette.cpp Palette/include/Palette.hpp UIFramework/libUIFramework.so
	$(MAKE) -C Palette

ToolManager/ToolManager.so: ToolManager/Makefile ToolManager/src/ToolManager.cpp ToolManager/include/ToolManager.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C ToolManager

ToolBar/libToolBar.so: ToolBar/Makefile ToolBar/src/Plugin.cpp ToolBar/src/ToolBar.cpp ToolBar/include/ToolBar.hpp ToolBar/include/ToolBarIID.hpp ToolManager/include/ToolManager.hpp UIFramework/libUIFramework.so
	$(MAKE) -C ToolBar

RectangleTool/libRectangleTool.so: RectangleTool/Makefile RectangleTool/src/Plugin.cpp RectangleTool/src/RectangleTool.cpp RectangleTool/src/RectangleShape.cpp RectangleTool/include/RectangleTool.hpp RectangleTool/include/RectangleShape.hpp ../../SDK/Interfaces/IDocument.hpp ../../SDK/Interfaces/IToolBar.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C RectangleTool

EllipseTool/libEllipseTool.so: EllipseTool/Makefile EllipseTool/src/Plugin.cpp EllipseTool/src/EllipseTool.cpp EllipseTool/src/EllipseShape.cpp EllipseTool/include/EllipseTool.hpp EllipseTool/include/EllipseShape.hpp ../../SDK/Interfaces/IDocument.hpp ../../SDK/Interfaces/IToolBar.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C EllipseTool

MurugovSelectTool/libMurugovSelectTool.so: MurugovSelectTool/Makefile MurugovSelectTool/src/Plugin.cpp MurugovSelectTool/src/SelectTool.cpp MurugovSelectTool/include/SelectTool.hpp ToolManager/include/ToolManager.hpp ../../SDK/Interfaces/IDocument.hpp ../../SDK/Interfaces/IToolManager.hpp
	$(MAKE) -C MurugovSelectTool

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
	$(MAKE) -C UIFramework clean
	$(MAKE) -C Palette clean
	$(MAKE) -C RectangleTool clean
	$(MAKE) -C EllipseTool clean
	$(MAKE) -C SelectorTool clean
	$(MAKE) -C MurugovSelectTool clean
	$(MAKE) -C ToolBar clean

-include $(DEPS)

.PHONY: all clean
