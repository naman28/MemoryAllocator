# Compiler and flags
CXX      := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Werror
INCLUDES := -Iinclude

# Build output directory
BUILD := build

# Build mode: release (default) or debug
MODE ?= release
ifeq ($(MODE),debug)
    CXXFLAGS += -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer -DDEBUG
    LDFLAGS  += -fsanitize=address,undefined
else
    CXXFLAGS += -O2 -DNDEBUG
endif

# ---------------------------------------------------------------------------
# Source files → object files
# ---------------------------------------------------------------------------

SRC_OBJS := \
    $(BUILD)/src/allocator.o       \
    $(BUILD)/src/heap.o            \
    $(BUILD)/src/freelist.o        \
    $(BUILD)/src/block.o           \
    $(BUILD)/src/bump_allocator.o  \
    $(BUILD)/src/stats.o

TEST_OBJS := \
    $(BUILD)/tests/test_main.o        \
    $(BUILD)/tests/test_bump.o        \
    $(BUILD)/tests/test_freelist.o    \
    $(BUILD)/tests/test_splitting.o   \
    $(BUILD)/tests/test_coalescing.o  \
    $(BUILD)/tests/test_alignment.o   \
    $(BUILD)/tests/test_realloc.o     \
    $(BUILD)/tests/test_stats.o       \
    $(BUILD)/tests/test_stl.o         \
    $(BUILD)/tests/test_stress.o

# ---------------------------------------------------------------------------
# Phony targets
# ---------------------------------------------------------------------------

.PHONY: all test bench example clean

all: test bench example

# ---------------------------------------------------------------------------
# Compilation rules
# ---------------------------------------------------------------------------

$(BUILD)/src/%.o: src/%.cpp | $(BUILD)/src
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/tests/%.o: tests/%.cpp | $(BUILD)/tests
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests -c $< -o $@

$(BUILD)/benchmark/%.o: benchmark/%.cpp | $(BUILD)/benchmark
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/examples/%.o: examples/%.cpp | $(BUILD)/examples
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ---------------------------------------------------------------------------
# Directory creation
# ---------------------------------------------------------------------------

$(BUILD)/src $(BUILD)/tests $(BUILD)/benchmark $(BUILD)/examples:
	mkdir -p $@

# ---------------------------------------------------------------------------
# Link executables
# ---------------------------------------------------------------------------

$(BUILD)/test_runner: $(SRC_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD)/bench_runner: $(SRC_OBJS) $(BUILD)/benchmark/bench_main.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD)/example: $(SRC_OBJS) $(BUILD)/examples/example_usage.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

# ---------------------------------------------------------------------------
# Run targets
# ---------------------------------------------------------------------------

test: $(BUILD)/test_runner
	@echo ""
	@echo "Running tests (MODE=$(MODE)) ..."
	@echo ""
	./$(BUILD)/test_runner

bench: $(BUILD)/bench_runner
	@echo ""
	@echo "Running benchmarks ..."
	@echo ""
	./$(BUILD)/bench_runner

example: $(BUILD)/example
	@echo ""
	@echo "Running example ..."
	@echo ""
	./$(BUILD)/example

clean:
	rm -rf $(BUILD)
