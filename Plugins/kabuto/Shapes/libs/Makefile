SRCS = libtess2/Source/bucketalloc.c \
       libtess2/Source/dict.c \
       libtess2/Source/geom.c \
       libtess2/Source/mesh.c \
       libtess2/Source/priorityq.c \
       libtess2/Source/sweep.c \
       libtess2/Source/tess.c

OBJS = $(SRCS:.c=.o)
TARGET = libtess2.a

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "[Archiving $(TARGET)]"
	ar rcs $(TARGET) $(OBJS)

%.o: %.c
	@echo "[Compiling $<]"
	clang -Ilibtess2/Include -fpic -g -Wall -Wextra -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
