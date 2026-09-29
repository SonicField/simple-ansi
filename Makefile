CC ?= cc
CPPFLAGS ?= -Isrc
CFLAGS ?= -O2
BASE_CFLAGS = -Wall -Wextra -Wshadow -Werror -std=c11
BUILD_DIR = build
TEST_TARGET = $(BUILD_DIR)/test_strip_ansi
PROPERTY_TARGET = $(BUILD_DIR)/test_properties

.PHONY: all test clean

all: $(TEST_TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(TEST_TARGET): tests/test_strip_ansi.c src/strip_ansi.c src/strip_ansi.h src/nbs_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(BASE_CFLAGS) $(CFLAGS) -o $@ tests/test_strip_ansi.c src/strip_ansi.c

$(PROPERTY_TARGET): tests/test_properties.c src/strip_ansi.c src/strip_ansi.h src/nbs_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(BASE_CFLAGS) $(CFLAGS) -o $@ tests/test_properties.c src/strip_ansi.c

test: $(TEST_TARGET) $(PROPERTY_TARGET)
	./$(TEST_TARGET)
	./$(PROPERTY_TARGET)

clean:
	rm -f $(TEST_TARGET) $(PROPERTY_TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true
