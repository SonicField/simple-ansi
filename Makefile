CC ?= cc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

CPPFLAGS ?= -Isrc
CFLAGS ?= -O2
BASE_CFLAGS = -Wall -Wextra -Wshadow -Werror -std=c11 \
	-D_POSIX_C_SOURCE=200809L
ALL_CFLAGS = $(BASE_CFLAGS) $(CFLAGS)
ANALYZER_CC ?= gcc
ANALYZER_CFLAGS ?= -O0 -g -fanalyzer

TARGET = simple-ansi
BUILD_DIR = build
UNIT_TEST_TARGET = $(BUILD_DIR)/test_strip_ansi
PROPERTY_TARGET = $(BUILD_DIR)/test_properties
IO_TEST_TARGET = $(BUILD_DIR)/test_io
UTF8_TEST_TARGET = $(BUILD_DIR)/test_utf8
ANALYZE_TARGET = $(BUILD_DIR)/simple-ansi-analyze
PIPE_HELPER = $(BUILD_DIR)/test_broken_pipe

CORE_SOURCES = src/ansi_io.c src/strip_ansi.c
SOURCES = src/main.c $(CORE_SOURCES)

.PHONY: all clean debug install test sanitize analyze

all: $(TARGET)

$(TARGET): $(SOURCES) src/ansi_io.h src/strip_ansi.h src/simple_assert.h
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $(SOURCES)

$(BUILD_DIR):
	mkdir -p $@

$(UNIT_TEST_TARGET): tests/test_strip_ansi.c src/strip_ansi.c src/strip_ansi.h src/simple_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ tests/test_strip_ansi.c src/strip_ansi.c

$(PROPERTY_TARGET): tests/test_properties.c src/strip_ansi.c src/strip_ansi.h src/simple_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ tests/test_properties.c src/strip_ansi.c

$(IO_TEST_TARGET): tests/test_io.c $(CORE_SOURCES) src/ansi_io.h src/strip_ansi.h src/simple_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ tests/test_io.c $(CORE_SOURCES)

$(UTF8_TEST_TARGET): tests/test_utf8.c src/strip_ansi.c src/strip_ansi.h src/simple_assert.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ tests/test_utf8.c src/strip_ansi.c

test: $(UNIT_TEST_TARGET) $(PROPERTY_TARGET) $(IO_TEST_TARGET) $(UTF8_TEST_TARGET) $(PIPE_HELPER) $(TARGET)
	./$(UNIT_TEST_TARGET)
	./$(PROPERTY_TARGET)
	./$(IO_TEST_TARGET)
	./$(UTF8_TEST_TARGET)
	PIPE_HELPER="$(CURDIR)/$(PIPE_HELPER)" ./tests/test_cli.sh "$(CURDIR)/$(TARGET)"

$(PIPE_HELPER): tests/test_broken_pipe.c | $(BUILD_DIR)
	$(CC) $(ALL_CFLAGS) -o $@ $<

install: $(TARGET)
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O0 -g3' all

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer' test

analyze: $(ANALYZE_TARGET)

$(ANALYZE_TARGET): $(SOURCES) src/ansi_io.h src/strip_ansi.h src/simple_assert.h | $(BUILD_DIR)
	$(ANALYZER_CC) $(CPPFLAGS) $(BASE_CFLAGS) $(ANALYZER_CFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(TARGET) $(UNIT_TEST_TARGET) $(PROPERTY_TARGET) $(IO_TEST_TARGET) $(UTF8_TEST_TARGET) $(ANALYZE_TARGET) $(PIPE_HELPER)
	rmdir $(BUILD_DIR) 2>/dev/null || true
