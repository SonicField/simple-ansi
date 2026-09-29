CC ?= cc
CPPFLAGS ?= -Isrc
CFLAGS ?= -O2
BASE_CFLAGS = -Wall -Wextra -Wshadow -Werror -std=c11
BUILD_DIR = build
TEST_TARGET = $(BUILD_DIR)/test_strip_ansi
PROPERTY_TARGET = $(BUILD_DIR)/test_properties
IO_TEST_TARGET = $(BUILD_DIR)/test_io
TARGET = simple-ansi

.PHONY: all test clean

all: $(TARGET)

$(TARGET): src/main.c src/ansi_io.c src/ansi_io.h src/strip_ansi.c src/strip_ansi.h src/nbs_assert.h
	$(CC) $(CPPFLAGS) $(BASE_CFLAGS) $(CFLAGS) -o $@ src/main.c src/ansi_io.c src/strip_ansi.c

$(BUILD_DIR):
	mkdir -p $@

$(TEST_TARGET): tests/test_strip_ansi.c src/strip_ansi.c src/strip_ansi.h src/nbs_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(BASE_CFLAGS) $(CFLAGS) -o $@ tests/test_strip_ansi.c src/strip_ansi.c

$(PROPERTY_TARGET): tests/test_properties.c src/strip_ansi.c src/strip_ansi.h src/nbs_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(BASE_CFLAGS) $(CFLAGS) -o $@ tests/test_properties.c src/strip_ansi.c

$(IO_TEST_TARGET): tests/test_io.c src/ansi_io.c src/ansi_io.h src/strip_ansi.c src/strip_ansi.h src/nbs_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(BASE_CFLAGS) $(CFLAGS) -o $@ tests/test_io.c src/ansi_io.c src/strip_ansi.c

test: $(TEST_TARGET) $(PROPERTY_TARGET) $(IO_TEST_TARGET) $(TARGET)
	./$(TEST_TARGET)
	./$(PROPERTY_TARGET)
	./$(IO_TEST_TARGET)
	./tests/test_cli.sh "$(CURDIR)/$(TARGET)"

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(PROPERTY_TARGET) $(IO_TEST_TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true
