CMAKE ?= cmake
BUILD_DIR ?= build

.PHONY: all debug release test check docker-check clean

all: debug

debug:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) --build $(BUILD_DIR)

release:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(BUILD_DIR)

test: debug
	ctest --test-dir $(BUILD_DIR) --output-on-failure

# Puerta de calidad completa. Ejecutar dentro de WSL2 o del contenedor dev.
check:
	./scripts/check.sh

# Misma puerta de calidad, sin toolchain en el host.
docker-check:
	docker compose --profile dev run --rm dev

clean:
	$(CMAKE) -E rm -rf $(BUILD_DIR)
