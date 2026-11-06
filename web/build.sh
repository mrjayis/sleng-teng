#!/usr/bin/env bash
# Builds web/teng40.wasm from the shared DSP engine.
# Needs clang (with the wasm32 target) and a WASI sysroot. On Ubuntu 24.04:
#   sudo apt-get install clang lld wasi-libc libc++-18-dev-wasm32 libclang-rt-18-dev-wasm32
set -euo pipefail
cd "$(dirname "$0")/.."

CXX="${CXX:-clang++}"
"$CXX" --target=wasm32-wasi -std=c++17 -O3 -flto \
    -fno-exceptions -fno-rtti \
    -mexec-model=reactor \
    -Idsp \
    dsp/Engine.cpp dsp/Upd931.cpp web/wasm_api.cpp \
    -Wl,--strip-all -Wl,--lto-O3 \
    -o web/teng40.wasm

echo "built web/teng40.wasm ($(wc -c < web/teng40.wasm) bytes)"
