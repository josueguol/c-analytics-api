# C Development Rules

La fuente normativa de las reglas de desarrollo del repositorio es [AGENTS.md](../AGENTS.md). Este documento existe para que las reglas sean visibles también para quienes no usan herramientas de agentes.

## Cumplimiento

Toda desviación debe justificarse antes de modificar código y registrarse en el cambio correspondiente. Las reglas cubren C17 portable, estructura modular, APIs encapsuladas, ownership de memoria, manejo de errores, pruebas, documentación y comprobaciones de entrega.

## Estado de cumplimiento

La implementación está dividida por responsabilidad y `src/main.c` sólo
gestiona el ciclo de vida de `PortalApp`. Las interfaces son opacas, están bajo
`include/portal_api/` y se documentan en [architecture.md](architecture.md).

El flujo `scripts/check.sh` compila con GCC y Clang, ejecuta pruebas con ASan,
UBSan y Valgrind cuando está disponible, verifica clang-format y ejecuta
clang-tidy.

La aplicación usa `libmicrohttpd`, `libpq` y `json-c` como dependencias de
sistema, declaradas explícitamente en CMake y Docker. No se copiarán sus fuentes
a `third_party/`: hacerlo impediría recibir sus actualizaciones de seguridad y
duplicaría mantenimiento. Esta es una desviación heredada que debe aprobarse o
reemplazarse por un mecanismo de dependencias vendorizadas antes de incorporar
otras dependencias externas. Esta es la única desviación documentada vigente.
