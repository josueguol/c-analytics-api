#!/bin/sh
#
# Instala en una distro Debian/Ubuntu (tipicamente WSL2) el toolchain que exige
# AGENTS.md: GCC, Clang, CMake, sanitizers, Valgrind, clang-format, clang-tidy y
# las bibliotecas de desarrollo de libmicrohttpd, libpq y json-c.
#
# Es idempotente: volver a ejecutarlo no rompe nada. No compila ni modifica el
# repositorio.

set -eu

if [ "$(id -u)" = "0" ]; then
    sudo=""
else
    sudo="sudo"
fi

packages="build-essential clang clang-format clang-tidy cmake pkg-config \
libmicrohttpd-dev libpq-dev libjson-c-dev valgrind jq curl git"

$sudo apt-get update

# El runtime de los sanitizers viene con clang en la mayoria de distros. Cuando
# se empaqueta aparte, el nombre depende de la version, asi que se agrega solo
# si existe en los repositorios configurados.
if apt-cache show libclang-rt-dev >/dev/null 2>&1; then
    packages="$packages libclang-rt-dev"
fi

# shellcheck disable=SC2086
$sudo apt-get install -y --no-install-recommends $packages

echo
echo "Toolchain instalado:"
gcc --version | head -n 1
clang --version | head -n 1
cmake --version | head -n 1
valgrind --version
clang-tidy --version | head -n 2 | tail -n 1
jq --version

for module in libmicrohttpd libpq json-c; do
    if pkg-config --exists "$module"; then
        echo "$module: $(pkg-config --modversion "$module")"
    else
        echo "$module: NO ENCONTRADO" >&2
        exit 1
    fi
done

echo
echo "Listo. Siguiente paso: ./scripts/check.sh"
