# Portal activity API (C + PostgreSQL)

API REST para registrar actividad de uno o varios portales, con cuentas opcionales. Está escrita en C17 usando `libmicrohttpd`, `json-c` y `libpq`; PostgreSQL conserva tanto las credenciales como los eventos.

## Inicio rápido

```sh
cp .env.example .env
docker compose up --build
curl http://localhost:8080/health
```

Si el puerto 8080 ya está ocupado, cambie `API_HOST_PORT` en `.env`; el puerto
interno del contenedor permanece en 8080.

### Compilación local

Instale CMake, un compilador C17 y las bibliotecas de desarrollo de
`libmicrohttpd`, `libpq` y `json-c`. Después compile siempre fuera de los
directorios de código fuente:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Las reglas obligatorias de C se documentan en [AGENTS.md](AGENTS.md), y las
responsabilidades y ownership de cada módulo en
[docs/architecture.md](docs/architecture.md).

La guía paso a paso de todos los endpoints está en la
[Wiki de pruebas](docs/wiki/API-TESTING.md).

Para ejecutar todas las comprobaciones locales (GCC, Clang, ASan, UBSan,
clang-format, clang-tidy, CTest y Valgrind cuando esté instalado):

```sh
./scripts/check.sh
```

Con los contenedores activos y `EXPOSE_CONFIRMATION_TOKEN=true`, la prueba de
integración completa se ejecuta con:

```sh
./tests/smoke_api.sh
```

La primera inicialización de PostgreSQL ejecuta `db/init.sql`. Para reiniciar por completo los datos de desarrollo, ejecute `docker compose down -v` (esto elimina el volumen de la base).

## Autenticación y confirmación

1. `POST /auth/register` crea la cuenta no confirmada.
2. En desarrollo, la respuesta incluye `confirmation_token`. En producción configure `EXPOSE_CONFIRMATION_TOKEN=false` e integre un proveedor de correo para entregar el código; el API nunca debe exponerlo al cliente final.
3. `POST /auth/confirm` consume el token.
4. `POST /auth/login` devuelve un token opaco de sesión. Envíelo como `Authorization: Bearer <token>`.
5. `DELETE /auth/session` revoca el token actual.

Las contraseñas se calculan en PostgreSQL con `pgcrypto` y bcrypt. Los tokens son UUID aleatorios, expiran y pueden revocarse; no contienen datos de usuario.

## Endpoints

| Método | Ruta | Auth | Propósito |
|---|---|---:|---|
| `GET` | `/health` | no | Liveness y conectividad de BD |
| `POST` | `/auth/register` | no | Alta de usuario |
| `POST` | `/auth/confirm` | no | Confirmación de cuenta |
| `POST` | `/auth/login` | no | Inicio de sesión |
| `DELETE` | `/auth/session` | sí | Cierre de sesión |
| `GET` | `/me/favorite-sections` | sí | Secciones favoritas |
| `PUT` / `DELETE` | `/me/favorite-sections/{key}` | sí | Añadir / quitar sección |
| `GET` | `/me/favorite-tags` | sí | Tags favoritos |
| `PUT` / `DELETE` | `/me/favorite-tags/{key}` | sí | Añadir / quitar tag |
| `POST` | `/activity` | opcional | Registra vista, click, like o comentario |
| `PUT` / `DELETE` | `/content/{id}/like` | opcional | Like idempotente / retirar like |
| `POST` | `/content/{id}/comments` | opcional | Crear comentario |
| `GET` | `/analytics/top-content?portal_key=x&days=7&limit=10` | sí | Notas más visitadas |

Los endpoints opcionales asocian el evento al usuario si llega una sesión válida. Sin sesión requieren `anonymous_id`, un UUID o identificador opaco generado y persistido por el cliente; no use correo, IP ni otros identificadores personales.

### Ejemplos

```sh
# Registrar actividad anónima
curl -X POST http://localhost:8080/activity \
  -H 'Content-Type: application/json' \
  -d '{"anonymous_id":"8de3c6d1-05bb-4b74-82bc-518f1f8b0871","portal_key":"mi-portal","event_type":"component_click","content_id":"nota-42","component_id":"hero-cta","page_url":"/notas/42","event_data":{"position":1}}'

# Alta y confirmación (el código sólo se entrega en modo desarrollo)
curl -X POST http://localhost:8080/auth/register -H 'Content-Type: application/json' \
  -d '{"email":"ana@example.com","password":"una-clave-larga","display_name":"Ana"}'
curl -X POST http://localhost:8080/auth/confirm -H 'Content-Type: application/json' \
  -d '{"token":"PEGAR_TOKEN"}'
```

## Notas operativas

- Configure una contraseña de PostgreSQL real, HTTPS delante del servicio y `EXPOSE_CONFIRMATION_TOKEN=false` antes de producción.
- `activity_events` separa el momento declarado (`occurred_at`) de la recepción. La primera fase registra vistas de página/contenido, clicks de componentes, likes y comentarios, con datos extra limitados a un objeto JSON.
- La API limita cuerpos a 1 MiB y no registra direcciones IP. Defina retención y consentimiento de analítica según la normativa aplicable.
