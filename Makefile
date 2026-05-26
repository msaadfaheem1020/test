.PHONY: all build run format help

all: build

build:
	cmake -B build
	cmake --build build -j

run: build
	./build/nbody

format:
	find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i

riscv-docker:
	./scripts/run-riscv.sh

help:
	@echo "Available commands:"
	@echo "  make build           - Build the project"
	@echo "  make run             - Build and run the executable"
	@echo "  make format          - Format all source files using clangd/clang-format"
	@echo "  make riscv-docker    - Start a riscv docker container and execute the loop"
	@echo "  make help            - Show this help message"
