#!/bin/sh

set -eu

cmake -S . -B build/gcc -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
cmake --build build/gcc --parallel
ctest --test-dir build/gcc --output-on-failure
if command -v valgrind >/dev/null 2>&1; then
    for test_binary in ./build/gcc/portal_validation_tests ./build/gcc/portal_http_json_tests; do
        valgrind --error-exitcode=1 --leak-check=full "${test_binary}"
    done
fi

# AddressSanitizer reserva su shadow memory en direcciones fijas. Con la entropia
# de ASLR de los kernels recientes (vm.mmap_rnd_bits=32, WSL2 6.x) las versiones
# antiguas de libsanitizer fallan al arrancar de forma intermitente: el binario
# muere con SIGSEGV antes de main y parece un bug del codigo.
if [ -r /proc/sys/vm/mmap_rnd_bits ]; then
    mmap_rnd_bits="$(cat /proc/sys/vm/mmap_rnd_bits)"
    if [ "${mmap_rnd_bits}" -gt 28 ]; then
        printf 'AVISO: vm.mmap_rnd_bits=%s. Si las pruebas con ASan fallan con SIGSEGV\n' \
            "${mmap_rnd_bits}" >&2
        printf '       intermitente, ejecute: sudo sysctl -w vm.mmap_rnd_bits=28\n' >&2
    fi
fi

cmake -S . -B build/clang -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
    -DPORTAL_ENABLE_SANITIZERS=ON
cmake --build build/clang --parallel
ctest --test-dir build/clang --output-on-failure
cmake --build build/clang --target format-check

cmake -S . -B build/tidy -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
    -DPORTAL_ENABLE_CLANG_TIDY=ON
cmake --build build/tidy --parallel
