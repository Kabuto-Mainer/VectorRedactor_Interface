TOOLS := $(patsubst %/Makefile,%,$(wildcard */Makefile))

.PHONY: all clean rebuild $(TOOLS)

all: $(TOOLS)

$(TOOLS):
	$(MAKE) -C $@

clean:
	@for dir in $(TOOLS); do \
		$(MAKE) -C $$dir clean; \
	done