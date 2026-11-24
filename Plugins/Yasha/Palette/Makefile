CXX := clang++
CPPFLAGS := -Iinclude -I.. -I../../../SDK
CXXFLAGS := -std=c++23 -fPIC -g -Wall -Wextra -MMD -MP
SOURCES := src/Plugin.cpp src/Palette.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)

all: libPalette.so

libPalette.so: $(OBJECTS) ../UIFramework/libUIFramework.so
	$(CXX) -shared $(OBJECTS) -L../UIFramework -lUIFramework -Wl,-rpath,'$$ORIGIN/../UIFramework' -o $@

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) libPalette.so

-include $(DEPS)

.PHONY: all clean
