CXX := clang++
CPPFLAGS := -Iinclude -I../ToolManager/include -I../ToolBar/include -I../../../SDK
CXXFLAGS := -std=c++23 -fPIC -g -Wall -Wextra -MMD -MP
SOURCES := src/Plugin.cpp src/SelectTool.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)

all: libMurugovSelectTool.so

libMurugovSelectTool.so: $(OBJECTS)
	$(CXX) -shared $^ -o $@

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) libMurugovSelectTool.so

-include $(DEPS)

.PHONY: all clean
