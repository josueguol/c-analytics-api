# Etapa reutilizable: solo herramientas. La usa la compilacion de produccion y
# tambien el servicio "dev" de docker-compose, que monta el codigo en /app.
FROM debian:bookworm-slim AS toolchain

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential clang clang-format \
       clang-tidy cmake pkg-config \
       libmicrohttpd-dev libpq-dev libjson-c-dev \
    && rm -rf /var/lib/apt/lists/*
RUN apt-get update \
    && apt-get install -y --no-install-recommends libclang-rt-14-dev valgrind curl jq \
    && rm -rf /var/lib/apt/lists/*

FROM toolchain AS build

WORKDIR /app
COPY CMakeLists.txt ./
COPY .clang-format .clang-tidy ./
COPY include ./include
COPY src ./src
COPY tests ./tests
RUN cmake -S . -B build-gcc -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc \
    && cmake --build build-gcc --parallel \
    && ctest --test-dir build-gcc --output-on-failure \
    && valgrind --error-exitcode=1 --leak-check=full \
       ./build-gcc/portal_validation_tests
RUN cmake -S . -B build-clang -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
       -DPORTAL_ENABLE_SANITIZERS=ON \
    && cmake --build build-clang --parallel \
    && ctest --test-dir build-clang --output-on-failure

FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends libmicrohttpd12 libpq5 libjson-c5 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --system --create-home app
COPY --from=build /app/build-gcc/portal-api /usr/local/bin/portal-api

USER app
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/portal-api"]
