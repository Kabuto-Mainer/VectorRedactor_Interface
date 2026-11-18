CXX      := clang++
CXXFLAGS := -std=c++23 -fPIC -g -Wall -Wextra
INCLUDES := -I../../../SDK -Iinclude -I.
SRCS     := src/Raylib_GUIEngine.cpp
BASE_NAME := libmurugov_Raylib_GUIEngine

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    TARGET  := $(BASE_NAME).dylib
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
    ifneq ($(BREW_PREFIX),)
        INCLUDES += -I$(BREW_PREFIX)/include
        LDFLAGS  += -L$(BREW_PREFIX)/lib
    endif

    LDFLAGS += -dynamiclib -undefined dynamic_lookup -lraylib \
               -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
    TARGET  := $(BASE_NAME).so
    LDFLAGS := -shared -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "[Building $(TARGET) on $(UNAME_S)]"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -rf $(BASE_NAME).so $(BASE_NAME).dylib *.dSYM

.PHONY: all clean
