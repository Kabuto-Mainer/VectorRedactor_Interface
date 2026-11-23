CXX       := clang++
CXXFLAGS  := -std=c++23 -fPIC -g -Wall -Wextra
INCLUDES  := -Iinclude -I../../../SDK -I../../Yasha/UIFramework -I.
SRCS      := ../../Yasha/UIFramework/Button.cpp ../../Yasha/UIFramework/ButtonPool.cpp ../../Yasha/UIFramework/IconButton.cpp src/ToolBar.cpp
BASE_NAME := libmurugov_ToolBar

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	TARGET  := $(BASE_NAME).dylib
	LDFLAGS := -dynamiclib -undefined dynamic_lookup
else
	TARGET  := $(BASE_NAME).so
	LDFLAGS := -shared
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "[Building $(TARGET) on $(UNAME_S)]"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -rf $(BASE_NAME).so $(BASE_NAME).dylib *.dSYM

.PHONY: all clean
