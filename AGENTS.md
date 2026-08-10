# Reglas obligatorias de desarrollo en C

Estas reglas aplican a cualquier cambio de código, prueba, documentación o configuración de este repositorio. Si una regla no puede cumplirse, se debe justificar explícitamente antes de implementar el cambio y dejar documentada la desviación.

## Objetivos

- Código mantenible, portable, seguro en memoria, fácil de probar y extender.
- Alta cohesión, bajo acoplamiento y una API clara.

## Lenguaje, compilación y herramientas

- Usar C17 sin extensiones específicas de compilador salvo aprobación explícita.
- Mantener compatibilidad con GCC y Clang, y con Linux, macOS y Windows cuando sea posible.
- Compilar sin warnings con `-Wall -Wextra -Wpedantic`; tratar warnings como errores cuando sea viable.
- Usar `clang-format`, `clang-tidy`, AddressSanitizer y UndefinedBehaviorSanitizer. Ejecutar Valgrind cuando esté disponible.
- Todo artefacto generado pertenece a `build/`; nunca junto al código fuente.

## Estructura y módulos

La estructura base es `include/`, `src/`, `tests/`, `docs/`, `resources/`, `scripts/`, `third_party/` y `build/`, con `CMakeLists.txt`, `README.md`, `LICENSE`, `.gitignore`, `.clang-format` y `.clang-tidy` en la raíz.

- Organizar `src/` por funcionalidad, nunca por tipo de archivo. Cada módulo tiene una responsabilidad única; no crear módulos genéricos como `utils`, `common`, `helpers` o `misc`.
- Las API públicas viven sólo bajo `include/portal_api/`, llevan el prefijo `portal_` y no exponen estructuras internas.
- Las cabeceras privadas quedan dentro de su módulo y no se incluyen desde otros módulos.
- `main.c` sólo inicializa, crea el contexto, ejecuta la aplicación y libera recursos.
- Preferir archivos de hasta 500 líneas; 800 es el máximo. Dividir funciones de más de 50 líneas preferentemente y de más de 100 obligatoriamente.

## Diseño y estilo

- No usar variables globales. El estado compartido pertenece a un contexto explícito.
- Usar `const` siempre que sea posible; usar `stdint.h`, `stddef.h` y `stdbool.h`, y preferir `size_t`, `uint32_t` e `int64_t` frente a `int` para tamaños o valores con ancho definido.
- Usar `snake_case`: funciones con verbo y prefijo de módulo, tipos en PascalCase y macros en `MAYUSCULAS_CON_GUIONES_BAJOS`.
- Declarar `static` toda función usada sólo en su archivo. Evitar includes innecesarios y preferir declaraciones adelantadas cuando basten.
- Preferir código simple y legible a abstracciones innecesarias. Optimizar sólo con evidencia de un cuello de botella.

## Memoria, errores y limpieza

- Toda reserva dinámica debe tener ownership documentado, comprobarse y liberarse exactamente una vez. Tras `free`, asignar `NULL` al puntero.
- Verificar todo valor de retorno y devolver errores claros. Documentar las condiciones de fallo de toda API que pueda fallar.
- Usar un único punto de limpieza (por ejemplo, `goto cleanup`) para evitar liberar recursos en rutas duplicadas.

## Pruebas y documentación

- Las pruebas viven sólo en `tests/`; nunca se mezclan con producción. Cada módulo importante debe tener pruebas.
- Documentar por módulo: responsabilidad, API pública, ownership de memoria y errores posibles.
- Las dependencias externas viven sólo en `third_party/` y nunca se modifican directamente.

## Lista de comprobación de entrega

Antes de entregar un cambio, comprobar: compilación sin warnings; fugas y ownership; tamaño de funciones y archivos; acoplamiento; encapsulación de la API pública; responsabilidad única; estructura de proyecto; pruebas relevantes y documentación actualizada.
