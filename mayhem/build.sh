#!/usr/bin/env bash
#
# mayhem/build.sh — build wangle's fuzz harnesses (sanitized, DWARF 3), their standalone reproducers,
# and the upstream gtest binaries (normal flags) that mayhem/test.sh runs.
#
# Runs offline in the commit image as `mayhem` in /mayhem. folly and fizz come prebuilt (static,
# uninstrumented) from /opt/toolchains/deps; everything else is an apt -dev package.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

FOLLY=/opt/toolchains/deps/folly-522d428132f00c169eeb0e60573e5dccad1017af
FIZZ=/opt/toolchains/deps/fizz-04b658f3f19fc71b31e93f4578cb477570fd9bde
PREFIX_PATH="$FOLLY;$FIZZ"

# Fresh build trees every run: a cache configured under a different compiler path (e.g. the ccache
# pre-fill wrapper) is reset by CMake and loses its -D settings, so re-runs would fail to configure.
TESTS_BUILD="$SRC/mayhem/build-tests"
FUZZ_BUILD="$SRC/mayhem/build-fuzz"
rm -rf "$TESTS_BUILD" "$FUZZ_BUILD"

# 1) Upstream test suite, normal flags, independent tree (so test.sh only runs it).
cmake -S wangle -B "$TESTS_BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$COVERAGE_FLAGS" \
  -DBUILD_TESTS=ON -DCMAKE_PREFIX_PATH="$PREFIX_PATH"
cmake --build "$TESTS_BUILD" -j"$MAYHEM_JOBS" --target CodecTest PipelineTest TLSCredProcessorTest

# 2) wangle + harnesses with sanitizers. fuzzer-no-link puts SanCov on wangle itself, so coverage
#    feedback reaches the fuzzed code; the harness executables link the engine (or the standalone
#    driver, compiled as C) and lsan_off.cc.
FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -O1 -fsanitize=fuzzer-no-link"
cmake -S mayhem -B "$FUZZ_BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=None \
  -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_FLAGS" \
  -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
  -DMAYHEM_OUT=/mayhem -DSTANDALONE_FUZZ_MAIN="$STANDALONE_FUZZ_MAIN" \
  -DFUZZ_ENGINE="$LIB_FUZZING_ENGINE"
cmake --build "$FUZZ_BUILD" -j"$MAYHEM_JOBS"
