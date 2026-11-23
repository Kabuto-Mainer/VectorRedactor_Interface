CXX := clang++
CPPFLAGS := -Iinclude -I.. -I../ToolManager/include -I../ToolBar/include -I../../../SDK
CXXFLAGS := -std=c++23 -fPIC -g -Wall -Wextra -MMD -MP
SOURCES := src/Plugin.cpp src/SelectorTool.cpp src/SelectionOverlay.cpp src/ShapeLookup.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)

all: libSelectorTool.so

libSelectorTool.so: $(OBJECTS)
	$(CXX) -shared $^ -o $@

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) libSelectorTool.so

-include $(DEPS)

.PHONY: all clean
