#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE_NAME="nbody-riscv-worker"

echo "Installing QEMU for RISCV64"
docker run --privileged --rm tonistiigi/binfmt --install riscv64

echo "Creating buildx builder"
docker buildx create --name riscv-builder --use 2>/dev/null \
  || docker buildx use riscv-builder

echo "Building RISCV64 image"
docker buildx build \
  --quiet \
  --platform linux/riscv64 \
  --tag "$IMAGE_NAME" \
  --output type=docker \
  "$PROJECT_ROOT"

echo "Running RISCV64 image"
docker run \
  --rm -t \
  --platform linux/riscv64 \
  --volume "$PROJECT_ROOT:/app" \
  --workdir /app \
  "$IMAGE_NAME" \
  bash -c '
    set -e
    echo "Compiling..."
    g++ -O2 -std=c++20 -pthread \
        -march=rv64gcv \
        -mabi=lp64d \
        src/nbody_riscv.cpp \
        src/compute_step.s \
        -o nbody_riscv

    echo "Running RISC-V program..."
    ./nbody_riscv

    echo "Finished"
  '