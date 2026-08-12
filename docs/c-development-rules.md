# C Development Rules

La fuente normativa de las reglas de desarrollo es [AGENTS.md](../AGENTS.md). Este documento existe para dar visibilidad a quienes no usan herramientas de agentes y para llevar el registro de desviaciones vigentes.

## Cumplimiento

Toda desviación debe justificarse antes de modificar código y registrarse aquí. Las reglas cubren C17 portable, estructura modular, APIs encapsuladas, ownership de memoria, manejo de errores, reglas específicas de HTTP y SQL, pruebas, documentación y comprobaciones de entrega.

## Estado de cumplimiento

La implementación está dividida por responsabilidad y `src/main.c` sólo gestiona el ciclo de vida de `PortalApp`. Las interfaces son opacas, viven en `include/portal_api/` y se documentan en [architecture.md](architecture.md).

`scripts/check.sh` es la puerta de calidad: compila con GCC y Clang, ejecuta las pruebas con ASan y UBSan, corre Valgrind cuando está disponible, verifica `clang-format` y ejecuta `clang-tidy`.

## Desviaciones vigentes

### 1. Dependencias de sistema en lugar de `third_party/`

`libmicrohttpd`, `libpq` y `json-c` se consumen como dependencias de sistema declaradas por `pkg-config` en CMake e instaladas en el `Dockerfile`. No se copian sus fuentes a `third_party/`: hacerlo impediría recibir sus actualizaciones de seguridad y duplicaría mantenimiento.

Estado: **aceptada** y recogida en la regla 39 de `AGENTS.md`. `third_party/` queda reservado para código vendorizado.

### 2. Portabilidad limitada a Linux

El servicio usa `_POSIX_C_SOURCE=200809L` y bibliotecas POSIX. Linux es la única plataforma **objetivo** verificada; macOS es best-effort y Windows no está soportado como destino de ejecución.

El **host de desarrollo** sí puede ser Windows: se trabaja desde WSL2 con `./scripts/setup-wsl.sh`, o desde el servicio `dev` de docker-compose. Ninguna de las dos vías introduce código específico de Windows; ambas ejecutan `scripts/check.sh` sin modificarlo.

Estado: **aceptada**, recogida en la regla 2 de `AGENTS.md`.

### 3. Sin capa de abstracción de memoria

Las asignaciones dinámicas están concentradas en `src/config/config.c`. No existe una capa `portal_memory_*`; introducirla hoy sería sobreingeniería.

Estado: **aceptada** con condiciones de revisión explícitas en la regla 29 de `AGENTS.md`.

### 4. Cobertura de pruebas parcial

Sólo `validation` tiene pruebas unitarias en C. El resto de módulos depende de PostgreSQL o de la red y se cubre mediante `tests/smoke_api.sh`.

Estado: **deuda técnica**. Se resuelve introduciendo dobles de prueba para `database` y `http` antes de agregar módulos nuevos con lógica propia.
