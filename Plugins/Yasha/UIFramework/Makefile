CXX := clang++
CPPFLAGS := -I.. -I../../../SDK
CXXFLAGS := -std=c++23 -fPIC -g -Wall -Wextra -MMD -MP
SOURCES := Plugin.cpp Button.cpp ButtonPool.cpp ColorButton.cpp IconButton.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)

all: libUIFramework.so

libUIFramework.so: $(OBJECTS)
	$(CXX) -shared $^ -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) libUIFramework.so

-include $(DEPS)

.PHONY: all clean
