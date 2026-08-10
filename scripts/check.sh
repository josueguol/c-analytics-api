#!/bin/sh

set -eu

cmake -S . -B build/gcc -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
cmake --build build/gcc --parallel
ctest --test-dir build/gcc --output-on-failure
if command -v valgrind >/dev/null 2>&1; then
    valgrind --error-exitcode=1 --leak-check=full ./build/gcc/portal_validation_tests
fi

cmake -S . -B build/clang -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
    -DPORTAL_ENABLE_SANITIZERS=ON
cmake --build build/clang --parallel
ctest --test-dir build/clang --output-on-failure
cmake --build build/clang --target format-check

cmake -S . -B build/tidy -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
    -DPORTAL_ENABLE_CLANG_TIDY=ON
cmake --build build/tidy --parallel
