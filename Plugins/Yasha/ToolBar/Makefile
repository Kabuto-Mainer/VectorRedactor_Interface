CXX := clang++
CPPFLAGS := -Iinclude -I.. -I../ToolManager/include -I../../../SDK
CXXFLAGS := -std=c++23 -fPIC -g -Wall -Wextra -MMD -MP
SOURCES := src/Plugin.cpp src/ToolBar.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)

all: libToolBar.so

libToolBar.so: $(OBJECTS) ../UIFramework/libUIFramework.so
	$(CXX) -shared $(OBJECTS) -L../UIFramework -lUIFramework -Wl,-rpath,'$$ORIGIN/../UIFramework' -o $@

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) libToolBar.so

-include $(DEPS)

.PHONY: all clean
