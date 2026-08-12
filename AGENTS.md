# Reglas obligatorias de desarrollo en C

Este documento es la **única fuente normativa** del repositorio. Aplica a todo cambio de código, prueba, documentación o configuración.

Si una regla no puede cumplirse por una razón técnica válida, la desviación debe justificarse antes de implementar el cambio y registrarse en `docs/c-development-rules.md`.

---

## 1. Principios generales

Prioridad, en este orden:

1. Correctitud.
2. Seguridad.
3. Legibilidad.
4. Mantenibilidad.
5. Simplicidad.
6. Rendimiento.

No introducir optimizaciones que compliquen el código sin evidencia de un cuello de botella real.

Aplicar: alta cohesión, bajo acoplamiento, responsabilidad única, interfaces pequeñas, dependencias explícitas, encapsulación, código simple antes que ingenioso.

Evitar sobrearquitectura y abstracciones innecesarias.

---

## 2. Estándar del lenguaje y portabilidad

Usar C17:

```cmake
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
```

No usar extensiones específicas de GCC, Clang o MSVC.

Sí está permitido —y es la práctica vigente— usar funciones POSIX habilitadas mediante el feature-test macro declarado en CMake:

```cmake
target_compile_definitions(portal_api_core PRIVATE _POSIX_C_SOURCE=200809L)
```

Esto cubre `strdup`, `strtok_r`, `localtime_r` y similares. Toda función POSIX usada debe estar cubierta por ese nivel.

Plataformas **objetivo** (dónde corre el servicio):

* **Linux**: soportado y verificado (`scripts/check.sh`, Docker).
* **macOS**: best-effort; no se rompe a propósito, no se verifica en cada cambio.
* **Windows**: no soportado. No agregar código ni reglas que finjan lo contrario.

Host de **desarrollo**: puede ser Linux, macOS o Windows. En Windows se trabaja
mediante WSL2 (`./scripts/setup-wsl.sh` deja la distro lista) o mediante el
servicio `dev` de docker-compose. Windows nativo no es objetivo de compilación:
no se agregan rutas de código, flags ni condicionales para MSVC ni MinGW.

Los archivos de texto se versionan y se materializan con LF; `.gitattributes` lo
impone. Un `.sh` con CRLF rompe la ejecución en WSL y en contenedor.

Mantener compatibilidad con GCC y Clang; ambos se compilan en `scripts/check.sh`.

---

## 3. Estructura del proyecto

```text
portal-activity-api/
│
├── AGENTS.md              ← reglas normativas
├── CMakeLists.txt
├── Makefile               ← atajos sobre CMake
├── README.md
├── LICENSE
├── Dockerfile
├── docker-compose.yml
├── .env.example
├── .gitignore
├── .dockerignore
├── .clang-format
├── .clang-tidy
│
├── include/portal_api/    ← API pública
├── src/                   ← implementación por módulo
├── tests/
├── db/                    ← esquema e inicialización SQL
├── docs/
│   ├── architecture.md
│   ├── c-development-rules.md
│   └── wiki/
├── resources/
├── scripts/
├── third_party/
└── build/
```

`build/` contiene únicamente artefactos generados y nunca se versiona.

---

## 4. Organización por módulos

Organizar `src/` por funcionalidad, nunca por tipo de archivo:

```text
src/
├── app/         app.c
├── config/      config.c
├── database/    database.c
├── http/        http.c request.c response.c authentication.c http_internal.h
│   └── routes/  auth.c favorites.c activity.c analytics.c
├── validation/  validation.c
└── main.c
```

No crear directorios para módulos triviales. La estructura crece con el proyecto.

---

## 5. API pública

Las interfaces públicas viven exclusivamente en `include/portal_api/` y llevan el prefijo `portal_`.

```text
include/portal_api/
├── app.h
├── config.h
├── database.h
├── http.h
└── validation.h
```

No exponer detalles internos.

---

## 6. Cabeceras privadas

Las cabeceras privadas quedan dentro de su módulo (`src/http/http_internal.h`) y no se incluyen desde otros módulos. El resto del código usa la API pública.

---

## 7. Encapsulación

Toda función usada sólo dentro de un archivo se declara `static`. No exportar símbolos innecesarios.

Cuando una estructura no necesita ser conocida externamente, usar tipo opaco:

```c
typedef struct PortalDatabase PortalDatabase;
```

La definición real permanece en el `.c`. `PortalApp`, `PortalConfig`, `PortalDatabase` y `PortalHttpServer` son opacos y deben seguir siéndolo.

---

## 8. Responsabilidad de módulos

Cada módulo representa una responsabilidad concreta (`config`, `database`, `http`, `validation`, `app`).

Prohibidos los módulos genéricos: `utils`, `helpers`, `common`, `misc`. Si aparece funcionalidad reutilizable, nombrarla por su responsabilidad (`string_utils`, `byte_buffer`).

---

## 9. Tamaño de archivos

Mantener archivos por debajo de ~500 líneas. Al acercarse a ~800, evaluar seriamente dividir responsabilidades.

Son indicadores de diseño, no reglas matemáticas. No dividir un archivo sólo para satisfacer una métrica.

---

## 10. Funciones

Cada función hace una tarea concreta. Referencia: idealmente menos de 50 líneas; revisar las que superen ~100.

No dividir artificialmente una función si empeora la legibilidad.

---

## 11. main.c

`src/main.c` permanece pequeño: inicializar, crear contexto, ejecutar, liberar, terminar.

```c
int main(int argc, char **argv)
{
    App app = {0};

    if (app_init(&app, argc, argv) != 0) {
        return EXIT_FAILURE;
    }

    int result = app_run(&app);

    app_destroy(&app);

    return result;
}
```

Sin lógica de negocio en `main.c`.

---

## 12. Variables

Inicializar toda variable antes de usarla:

```c
int count = 0;
char *buffer = NULL;
FILE *file = NULL;
```

Nunca depender del contenido previo de memoria no inicializada.

---

## 13. Estado global

Evitar estado global **mutable**. El estado compartido pertenece a un contexto explícito que se pasa a los módulos que lo necesitan (`PortalApp`, `PortalHttpContext`).

Las constantes globales inmutables sí pueden usarse.

---

## 14. Const correctness

Usar `const` siempre que una función no necesite modificar un dato. La firma debe expresar qué puede modificar.

```c
const char *portal_config_database_url(const PortalConfig *config);
```

---

## 15. Tipos

Usar `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`.

`size_t` para tamaños, capacidades, índices y cantidades de memoria. Anchos explícitos (`uint32_t`, `int64_t`) cuando el ancho importa. No usar `int` indiscriminadamente para tamaños.

---

## 16. Manejo de errores

Comprobar el valor de retorno de toda operación que pueda fallar: `malloc`, `calloc`, `realloc`, `strdup`, `fopen`, `snprintf`, `PQexecParams`, `MHD_start_daemon`, `json_object_*`, `strtol`, etc.

No ignorar errores silenciosamente. Toda API que pueda fallar documenta cómo comunica el error.

---

## 17. Gestión de recursos

Todo recurso adquirido tiene un propietario y un mecanismo claro de liberación: memoria, archivos, sockets, mutexes, threads, handles, conexiones `PGconn`, resultados `PGresult`, objetos `json-c`, daemons MHD.

Adquisición y liberación simétricas:

```text
malloc          → free
fopen           → fclose
PQexecParams    → PQclear
json_object_new → json_object_put
MHD_start_daemon→ MHD_stop_daemon
xxx_create      → xxx_destroy
```

---

## 18. Política de memoria

No se aceptan: memory leaks, double free, use-after-free, lecturas o escrituras inválidas, buffer overflows, acceso a memoria no inicializada.

---

## 19. Ownership

Cada bloque de memoria tiene un propietario identificable. Toda API con punteros deja claro si entrega, recibe, mantiene o presta ownership.

```c
/**
 * Ownership: el llamador recibe ownership y debe llamar a portal_database_result_destroy().
 */
PortalDatabaseResult *portal_database_query(PortalDatabase *database, ...);

/**
 * Ownership: referencia interna. El llamador NO libera. Válida mientras viva Config.
 */
const char *portal_config_database_url(const PortalConfig *config);
```

No dejar ownership ambiguo. `docs/architecture.md` mantiene la tabla de ownership por módulo y debe actualizarse cuando cambie.

---

## 20. Reserva dinámica

Toda reserva se comprueba:

```c
char *buffer = malloc(size);

if (buffer == NULL) {
    return ERROR_OUT_OF_MEMORY;
}
```

Igual para `calloc()`, `realloc()`, `strdup()`.

---

## 21. Liberación

Toda memoria con ownership se libera exactamente una vez. No llamar `free()` sobre memoria ya liberada, perteneciente a otro objeto, en stack, o prestada.

---

## 22. NULL después de free

Si el puntero sigue existiendo y podría reutilizarse por accidente:

```c
free(buffer);
buffer = NULL;
```

**No es obligatorio** si el puntero sale inmediatamente de alcance y no puede volver a usarse. `ptr = NULL` no sustituye a un ownership correcto.

---

## 23. realloc

Nunca sobrescribir el único puntero válido sin considerar el fallo:

```c
void *new_buffer = realloc(buffer, new_size);

if (new_buffer == NULL) {
    /* buffer sigue siendo válido */
    return ERROR_OUT_OF_MEMORY;
}

buffer = new_buffer;
```

El fallo de `realloc()` no libera el bloque original; la función decide explícitamente si conservarlo o liberarlo en su cleanup.

---

## 24. Overflow al calcular tamaños

Antes de reservar memoria basada en multiplicaciones, validar:

```c
if (count > SIZE_MAX / sizeof(Item)) {
    return ERROR_OVERFLOW;
}

Item *items = malloc(count * sizeof(Item));
```

Crítico cuando los tamaños vienen de red, cuerpo HTTP, base de datos, archivos o entrada de usuario.

---

## 25. Stack vs heap

No usar heap innecesariamente. Para datos pequeños, locales y de tamaño conocido, preferir stack (`char buffer[256];`). Evitar objetos muy grandes en stack.

Elegir según tamaño, lifetime, ownership, recursividad y plataforma.

---

## 26. Inicialización de memoria

No leer memoria sin inicializar. Usar `calloc()` sólo cuando la inicialización a cero se necesita realmente; no si todo el contenido se sobrescribirá después.

---

## 27. Destructores

Toda estructura que posea recursos ofrece destructor explícito. Si existe `xxx_create()`, normalmente existe `xxx_destroy()` o una operación equivalente documentada.

`xxx_destroy(NULL)` debe ser seguro.

---

## 28. Cleanup centralizado

Cuando una función adquiere varios recursos, usar un único flujo de limpieza:

```c
int process_file(const char *path)
{
    int result = ERROR;
    FILE *file = NULL;
    char *buffer = NULL;

    file = fopen(path, "rb");

    if (file == NULL) {
        goto cleanup;
    }

    buffer = malloc(4096);

    if (buffer == NULL) {
        goto cleanup;
    }

    /* procesamiento */

    result = SUCCESS;

cleanup:
    free(buffer);

    if (file != NULL) {
        fclose(file);
    }

    return result;
}
```

`goto cleanup` es aceptable cuando simplifica la liberación. No usar `goto` para flujo arbitrario.

---

## 29. Abstracción de memoria

**No existe ni se exige hoy una capa `memory_*`.** Las asignaciones dinámicas están concentradas en un solo módulo (`src/config/config.c`); crear una capa ahora sería sobreingeniería (§52).

Regla vigente: `malloc`, `calloc`, `realloc`, `strdup` y `free` directos, con ownership documentado.

Introducir una capa centralizada (`portal_memory_alloc`, …) sólo cuando se cumpla alguna condición:

* aparece una tercera fuente independiente de asignaciones;
* se necesita instrumentación, estadísticas o inyección de fallos de asignación en tests;
* se planea cambiar de allocator.

Si se introduce, es una sola capa para todo el proyecto, nunca varias equivalentes.

---

## 30. Buffers

Todo buffer conoce su capacidad. Cuando corresponda, modelarlo explícitamente:

```c
typedef struct {
    unsigned char *data;
    size_t length;
    size_t capacity;
} ByteBuffer;
```

Nunca asumir implícitamente el tamaño disponible.

---

## 31. Strings

Las cadenas C necesitan terminador `'\0'`: una cadena de longitud `length` necesita al menos `length + 1` bytes. Validar overflow antes de sumar o multiplicar tamaños.

---

## 32. Funciones inseguras

Prohibido `gets()`. Evitar `strcpy()`, `strcat()`, `sprintf()` sin garantía explícita del tamaño de destino.

Preferir `snprintf()` y **verificar siempre** el retorno, tanto error como truncamiento:

```c
int written = snprintf(dest, sizeof(dest), "%s", source);

if (written < 0 || (size_t)written >= sizeof(dest)) {
    return ERROR_TRUNCATED;
}
```

---

## 33. Includes

Cada `.c` incluye primero su propia cabecera pública o principal, luego cabeceras del proyecto, luego del sistema:

```c
#include "portal_api/validation.h"

#include <stdbool.h>
#include <string.h>
```

Esto detecta dependencias implícitas.

---

## 34. Cabeceras autocontenidas

Todo `.h` puede incluirse por sí solo. Si usa `size_t`, incluye `<stddef.h>`. No depender del orden accidental de includes.

Include guards con prefijo del proyecto:

```c
#ifndef PORTAL_API_VALIDATION_H
#define PORTAL_API_VALIDATION_H

/* ... */

#endif
```

---

## 35. Dependencias entre cabeceras

No incluir cabeceras innecesariamente. Usar forward declarations cuando basten:

```c
typedef struct PortalDatabase PortalDatabase;
```

Las cabeceras públicas no exponen tipos de `libpq`, `libmicrohttpd` ni `json-c`.

---

## 36. Naming

* Archivos y funciones: `snake_case`, con verbo y prefijo de módulo.
* Tipos: `PascalCase` con prefijo `Portal` en la API pública.
* Macros y constantes de preprocesador: `MAYUSCULAS_CON_GUIONES_BAJOS`.

```c
portal_http_route_top_content()
PortalDatabaseResult
PORTAL_MAX_BODY_SIZE
```

---

## 37. Prefijos de API

Toda API pública usa el prefijo `portal_` / `Portal`. C no tiene namespaces; símbolos globales genéricos como `create()` o `parse()` están prohibidos.

---

## 38. Números mágicos

Evitar números sin significado explícito. Cuando el valor tenga significado de dominio, nombrarlo:

```c
enum {
    PORTAL_MAX_BODY_SIZE = 1048576
};
```

No crear constantes para valores triviales evidentes.

---

## 39. Dependencias externas

Las dependencias actuales (`libmicrohttpd`, `libpq`, `json-c`) son **dependencias de sistema** declaradas por `pkg-config` en CMake e instaladas en el `Dockerfile`. Esto es válido y deliberado: permite recibir sus actualizaciones de seguridad sin mantener copias.

`third_party/` se reserva para código vendorizado. Si algo se vendoriza:

* nunca se mezcla con `src/`;
* no se modifica salvo necesidad documentada;
* los parches propios quedan claramente identificados.

Toda dependencia nueva se declara simultáneamente en `CMakeLists.txt`, `Dockerfile` y `README.md`.

---

## 40. Reglas específicas del servicio HTTP + SQL

Estas reglas no son C genérico; son obligatorias en este proyecto.

**SQL**

* Sólo consultas parametrizadas (`PQexecParams`). Prohibido construir SQL concatenando entrada.
* Nunca devolver el texto de error de libpq al cliente; va al log, al cliente va un error genérico.

**Secretos**

* Sólo por variables de entorno. Nunca loguear `DATABASE_URL`, contraseñas, tokens de sesión ni de confirmación.
* `.env` nunca se versiona; `.env.example` no contiene valores reales.
* `EXPOSE_CONFIRMATION_TOKEN=true` es exclusivo de desarrollo.

**Entrada HTTP**

* Límite explícito de tamaño de cuerpo (hoy 1 MiB); superarlo se rechaza sin reservar memoria adicional.
* Validar antes de usar: nunca asumir JSON bien formado, campos presentes ni tipos correctos.
* Toda entrada inválida produce 4xx con mensaje estable, no un fallo del proceso.

**Parsing numérico**

* `strtol`/`strtoll` con `errno = 0`, comprobación de `endptr` y de rango. `atoi` prohibido.
* Patrón de referencia: `src/http/routes/analytics.c` (`analytics_parse_range`).

**Errores hacia el cliente**

* Mapa explícito de error interno a código HTTP. El detalle interno sólo va al log.

**Concurrencia y reentrancia**

* En handlers no usar funciones no reentrantes: `strtok`, `localtime`, `gmtime`, `asctime`, `strerror`. Usar las variantes `_r`.
* `PGconn` no se comparte entre hilos. El servidor usa el hilo de polling interno de libmicrohttpd sobre una única conexión. Antes de habilitar múltiples workers hay que introducir un pool de conexiones explícito.

**Aleatoriedad**

* Tokens de sesión y confirmación desde un CSPRNG (`gen_random_uuid()` de pgcrypto o el CSPRNG del sistema). `rand()` prohibido para cualquier valor con función de seguridad.

**Datos personales**

* No registrar IP ni correo en eventos de actividad. `anonymous_id` es un identificador opaco del cliente.

---

## 41. Testing

Las pruebas viven sólo en `tests/`, nunca mezcladas con producción.

* Pruebas unitarias en C, un binario por área, registrado en CMake con `add_test`. Nombre `tests/test_<modulo>.c`; si un módulo crece, `tests/<modulo>/test_<tema>.c`.
* Pruebas de integración end-to-end en shell (`tests/smoke_api.sh`), requieren los contenedores activos.
* Todo binario de test compila con los mismos warnings y sanitizers que el código de producción.

Cobertura mínima exigible: todo módulo sin dependencias externas (`validation`, y cualquier lógica pura nueva) tiene pruebas unitarias. Los módulos que dependen de PostgreSQL o de la red se cubren por `smoke_api.sh` hasta que exista un doble de prueba.

Probar especialmente: condiciones límite, errores, entradas inválidas, buffers vacíos, tamaños cero, valores máximos, ownership, creación/destrucción y fallos de asignación cuando puedan simularse.

---

## 42. Compilación

El proyecto compila sin warnings.

Con GCC/Clang, siempre y en todos los targets (biblioteca, ejecutable y tests):

```text
-Wall -Wextra -Wpedantic -Werror
```

`-Werror` no es opcional ni exclusivo de CI: ya está activo en `CMakeLists.txt`.

Las opciones son específicas por compilador; no pasar flags de GCC a otros compiladores.

---

## 43. Sanitizers

`-DPORTAL_ENABLE_SANITIZERS=ON` habilita AddressSanitizer y UndefinedBehaviorSanitizer.

Los flags de sanitizer se aplican **en compilación y en enlace, a todos los targets** (biblioteca, ejecutable y tests). Un target instrumentado sólo al enlazar no detecta nada.

---

## 44. Análisis de memoria

Las pruebas con ASan deben quedar libres de: heap-buffer-overflow, stack-buffer-overflow, use-after-free, double-free, invalid free y cualquier acceso inválido detectable.

En Linux, complementar con Valgrind (`--leak-check=full --error-exitcode=1`) para fugas.

---

## 45. Análisis estático

`clang-tidy` con el `.clang-tidy` del repositorio. Configuración vigente:

```yaml
Checks: >
  -*,
  bugprone-*,
  -bugprone-easily-swappable-parameters,
  cert-*,
  clang-analyzer-*,
  -clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,
  misc-const-correctness,
  performance-*
WarningsAsErrors: '*'
```

`HeaderFilterRegex` debe cubrir las cabeceras públicas **y** las privadas de `src/`; si no, las cabeceras internas quedan sin analizar.

Evaluar cada regla antes de habilitarla globalmente. No introducir reglas con muchos falsos positivos y poco beneficio.

---

## 46. Formato

`.clang-format` en el repositorio; todo el código lo sigue. `cmake --build build --target format` aplica, `format-check` verifica.

No discutir formato manualmente cuando `clang-format` puede resolverlo.

---

## 47. CMake

Toda la configuración de compilación vive en CMake.

* Sin listas de archivos ni bloques de flags duplicados: una sola lista de fuentes (`PORTAL_API_SOURCES`) y un target `INTERFACE` con los flags comunes, reutilizado por todos los targets.
* Separar aplicación, biblioteca interna y tests. La lógica vive en `portal_api_core`; `portal_api` sólo enlaza `src/main.c`.
* Toda fuente o cabecera nueva se agrega a la lista correspondiente en el mismo cambio.

---

## 48. Build fuera del árbol de código

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Nunca generar `.o`, `.a`, `.so` ni ejecutables dentro de `src/`.

---

## 49. Puerta de calidad

`scripts/check.sh` es la comprobación obligatoria antes de entregar un cambio. Ejecuta: build con GCC (Release) y Clang (Debug con ASan/UBSan), CTest en ambos, Valgrind si está disponible, `format-check` y `clang-tidy`.

Se ejecuta desde Linux, desde WSL2 (`make check`) o desde el contenedor de desarrollo (`make docker-check`, es decir `docker compose --profile dev run --rm dev`). Las tres vías corren el mismo script.

Si se agrega CI, ejecuta exactamente ese script; no se duplica la lógica de comprobación.

---

## 50. Documentación

Cada módulo importante documenta: responsabilidad, API pública, ownership, lifetime, errores, restricciones e invariantes.

* `docs/architecture.md`: módulos, ownership y dependencias. Se actualiza cuando cambia el ownership o se agrega un módulo.
* `docs/wiki/API-TESTING.md`: endpoints paso a paso. Se actualiza cuando cambia un endpoint.
* `README.md`: objetivo, dependencias, compilación, ejecución, tests, sanitizers, estructura, endpoints y notas operativas.

No documentar lo obvio. Documentar lo que no se deduce del código.

---

## 51. Regla de modificación

Antes de escribir código nuevo:

1. Revisar el módulo afectado.
2. Identificar la responsabilidad correcta.
3. Evitar duplicar funcionalidad existente.
4. Identificar el ownership de todo recurso involucrado.
5. Determinar cómo se manejarán los errores.
6. Determinar qué pruebas hacen falta.

No empezar creando archivos o abstracciones sin entender dónde pertenece el cambio.

---

## 52. No sobreingeniería

No crear por defecto factories, managers, service locators, contenedores de dependencias, abstracciones genéricas, wrappers ni capas sin necesidad concreta y actual.

C se mantiene explícito y simple.

---

## 53. Definition of Done

**Compilación**

* [ ] Compila con GCC y Clang sin warnings, con `-Werror`.
* [ ] Respeta C17 (+ POSIX declarado).

**Arquitectura**

* [ ] El código está en el módulo correcto.
* [ ] No agrega dependencias innecesarias.
* [ ] No expone detalles internos en `include/portal_api/`.
* [ ] Alta cohesión, bajo acoplamiento.

**Memoria**

* [ ] Cada asignación tiene ownership claro y documentado.
* [ ] Toda memoria se libera exactamente una vez.
* [ ] Sin double free, use-after-free ni dangling pointers.
* [ ] Sin accesos fuera de límites.
* [ ] Se comprobaron los fallos de asignación.
* [ ] `realloc()` manejado de forma segura.
* [ ] Se consideró overflow al calcular tamaños.
* [ ] Los destructores liberan todo lo que poseen.

**Recursos**

* [ ] Archivos, sockets, handles y locks liberados.
* [ ] `PGresult` liberado con `PQclear`; objetos `json-c` con `json_object_put`.
* [ ] Todo recurso adquirido tiene cleanup.

**Servicio**

* [ ] Consultas SQL parametrizadas.
* [ ] Entrada HTTP validada y con límite de tamaño.
* [ ] Ningún secreto ni detalle interno en la respuesta o el log.
* [ ] Retornos de `snprintf` verificados (error y truncamiento).

**Calidad**

* [ ] Responsabilidades claras, sin duplicación innecesaria.
* [ ] Sin abstracciones innecesarias.
* [ ] API pública mínima.
* [ ] Ownership y lifetime documentados cuando no son obvios.

**Testing**

* [ ] Pruebas para el comportamiento nuevo y para los errores relevantes.
* [ ] `./scripts/check.sh` pasa completo.
* [ ] ASan y UBSan sin reportes.
* [ ] Sin fugas conocidas.

---

## Instrucciones para el agente

1. Analizar primero la arquitectura existente.
2. No reorganizar código no relacionado con la tarea.
3. Respetar estas reglas en todo código nuevo o modificado.
4. No hacer refactors masivos sin necesidad.
5. Identificar explícitamente ownership y lifetime al introducir punteros o recursos.
6. Implementar el cambio más simple que satisfaga el requerimiento.
7. Agregar o actualizar pruebas.
8. Compilar con warnings habilitados.
9. Ejecutar tests.
10. Ejecutar sanitizers cuando estén disponibles.
11. Corregir lo encontrado antes de dar por terminada la tarea.

Si se descubre una violación existente que no hace falta corregir para completar la tarea: no refactorizar sin que lo pidan, documentar el problema y proponerlo como trabajo separado si representa un riesgo real.

No afirmar que una tarea está terminada si hay errores de memoria conocidos, comportamiento indefinido, tests fallando o errores de compilación.

---

## Principio final

En C, la gestión de recursos forma parte del diseño de la aplicación. Para cada recurso debe poder responderse:

```text
¿Quién lo crea?
        ↓
¿Quién es su propietario?
        ↓
¿Cuánto tiempo vive?
        ↓
¿Quién puede utilizarlo?
        ↓
¿Quién lo libera?
        ↓
¿Cuándo se libera?
```

Si alguna pregunta no tiene respuesta clara, el diseño se revisa antes de continuar.
