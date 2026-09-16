CC ?= cc
COVERAGE_CC ?= gcc
ANALYZER_CC ?= gcc
GCOV ?= gcov
CLANG_FORMAT ?= clang-format
ASAN_OPTIONS ?= detect_leaks=0
CPPFLAGS ?=
CFLAGS ?= -O2
LDFLAGS ?=
LDLIBS ?=
TEST_TIMEOUT ?= 5

TARGET := morse
MODULES := main application morse_tree morse_table morse_decoder
SOURCES := $(addprefix src/,$(addsuffix .c,$(MODULES)))
HEADERS := $(wildcard include/*.h)
FORMAT_SOURCES := $(SOURCES) $(HEADERS) tests/test_modules.c
COMMON_FLAGS := -Iinclude -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
SANITIZER_FLAGS := -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined
COVERAGE_FLAGS := -g -O0 --coverage

.PHONY: all test sanitize coverage analyze format check-format clean FORCE
all: $(TARGET)

# Separate profiles prevent release, test, sanitizer, and coverage objects from mixing.
# A content-checked configuration stamp also rebuilds after compiler or flag changes.
define build_profile
$(1)_OBJECTS := $$(addprefix build/$(1)/objects/,$$(addsuffix .o,$$(MODULES)))

build/$(1)/configuration: FORCE
	@mkdir -p $$(@D)
	@printf '%s\n' '$(2)' '$(3)' '$(CPPFLAGS)' '$(COMMON_FLAGS)' '$(LDFLAGS)' '$(LDLIBS)' > $$@.tmp
	@if cmp -s $$@ $$@.tmp; then rm -f $$@.tmp; else mv $$@.tmp $$@; fi

build/$(1)/objects/%.o: src/%.c build/$(1)/configuration
	@mkdir -p $$(@D)
	$(2) $(CPPFLAGS) $(COMMON_FLAGS) $(3) -MMD -MP -c $$< -o $$@

build/$(1)/objects/test_modules.o: tests/test_modules.c build/$(1)/configuration
	@mkdir -p $$(@D)
	$(2) $(CPPFLAGS) $(COMMON_FLAGS) $(3) -MMD -MP -c $$< -o $$@

build/$(1)/$(TARGET): $$($(1)_OBJECTS)
	$(2) $(3) $$^ $(LDFLAGS) $(LDLIBS) -o $$@

build/$(1)/module-tests: build/$(1)/objects/test_modules.o $$(filter-out build/$(1)/objects/main.o,$$($(1)_OBJECTS))
	$(2) $(3) $$^ $(LDFLAGS) $(LDLIBS) -o $$@

-include $$($(1)_OBJECTS:.o=.d) build/$(1)/objects/test_modules.d
endef

$(eval $(call build_profile,release,$(CC),$(CFLAGS)))
$(eval $(call build_profile,test,$(CC),-g -O0))
$(eval $(call build_profile,sanitize,$(CC),$(SANITIZER_FLAGS)))
$(eval $(call build_profile,coverage,$(COVERAGE_CC),$(COVERAGE_FLAGS)))
$(eval $(call build_profile,analyzer,$(ANALYZER_CC),-g -O0 -fanalyzer))

$(TARGET): build/release/$(TARGET)
	cp $< $@

test: $(TARGET) build/test/$(TARGET) build/test/module-tests
	timeout 30 build/test/module-tests
	TEST_TIMEOUT=$(TEST_TIMEOUT) sh tests/run_baseline.sh ./build/test/$(TARGET)

sanitize: build/sanitize/$(TARGET) build/sanitize/module-tests
	ASAN_OPTIONS=$(ASAN_OPTIONS) timeout 30 build/sanitize/module-tests
	ASAN_OPTIONS=$(ASAN_OPTIONS) TEST_TIMEOUT=$(TEST_TIMEOUT) sh tests/run_baseline.sh ./build/sanitize/$(TARGET)

coverage: build/coverage/$(TARGET) build/coverage/module-tests
	rm -f $(coverage_OBJECTS:.o=.gcda) build/coverage/objects/test_modules.gcda
	timeout 30 build/coverage/module-tests
	TEST_TIMEOUT=$(TEST_TIMEOUT) sh tests/run_baseline.sh ./build/coverage/$(TARGET)
	$(GCOV) -n -b -c $(coverage_OBJECTS:.o=.gcno)

analyze: $(analyzer_OBJECTS)

format:
	$(CLANG_FORMAT) -i $(FORMAT_SOURCES)

check-format:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_SOURCES)

clean:
	rm -rf -- build/release build/test build/sanitize build/coverage build/analyzer
	rm -f -- $(TARGET)

FORCE:
