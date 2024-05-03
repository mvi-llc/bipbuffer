.PHONY: all build test benchmark tidy clean

all: build

# Build
build:
	cmake --preset default
	cmake --build build

# Test
test: build
	ctest --preset default

# Benchmark
benchmark: build
	ctest --preset benchmark

# Run clang-tidy on host
tidy: build
	clang-tidy -p build --config-file=.clang-tidy --use-color $$(find src/ -name '*.cpp')

# Clean up build directory
clean:
	rm -rf build
