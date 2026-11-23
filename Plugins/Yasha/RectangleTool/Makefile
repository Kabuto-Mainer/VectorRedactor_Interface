CXX := clang++
CPPFLAGS := -Iinclude -I.. -I../Document/include -I../Palette/include -I../ToolBar/include -I../../../SDK
CXXFLAGS := -std=c++23 -fPIC -g -Wall -Wextra -MMD -MP
SOURCES := src/Plugin.cpp src/RectangleTool.cpp src/RectangleShape.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)

all: libRectangleTool.so

libRectangleTool.so: $(OBJECTS)
	$(CXX) -shared $^ -o $@

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) libRectangleTool.so

-include $(DEPS)

.PHONY: all clean
