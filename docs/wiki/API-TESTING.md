# Wiki de pruebas de endpoints

Guía reproducible para levantar la API y probar todos sus endpoints con
`curl`. Está dirigida al entorno Docker de desarrollo.

## 1. Preparación

Requisitos: Docker Compose, `curl` y `jq`.

```sh
cp .env.example .env
```

Para recibir el token de confirmación directamente en la respuesta durante las
pruebas, conserva `EXPOSE_CONFIRMATION_TOKEN=true`. En producción el token debe
enviarse por correo y esa opción debe estar desactivada.

Si el puerto 8080 ya está ocupado, cambia el puerto del host en `.env`:

```env
API_HOST_PORT=18080
```

## 2. Levantar y detener

```sh
docker compose up --build -d
export BASE_URL=http://127.0.0.1:8080
```

Con `API_HOST_PORT=18080`, usa `export BASE_URL=http://127.0.0.1:18080`.

Detener conservando datos:

```sh
docker compose down
```

Reiniciar PostgreSQL desde cero, eliminando usuarios, sesiones y eventos:

```sh
docker compose down -v
```

Todas las peticiones JSON de esta guía usan `Content-Type: application/json`.
Las rutas protegidas usan `Authorization: Bearer <ACCESS_TOKEN>`.

```sh
export ANONYMOUS_ID=8de3c6d1-05bb-4b74-82bc-518f1f8b0871
```

`ANONYMOUS_ID` es un identificador opaco generado por el cliente; no debe ser
correo, IP ni otro dato personal.

## 3. Salud

### `GET /health`

Verifica que la API puede conectarse a PostgreSQL.

```sh
curl -i "$BASE_URL/health"
```

Esperado: `200 OK` y `{"status":"ok"}`. Si la base no está disponible,
responde `503 database_unavailable`.

## 4. Cuenta y sesiones

### `POST /auth/register`

Crea una cuenta no confirmada. `password` debe tener al menos 12 caracteres.

```sh
export EMAIL=ana@example.com
export PASSWORD=correct-horse-battery
REGISTRATION_JSON="$(curl -fsS -X POST "$BASE_URL/auth/register" \
  -H 'Content-Type: application/json' \
  -d "{\"email\":\"$EMAIL\",\"password\":\"$PASSWORD\",\"display_name\":\"Ana\"}")"
printf '%s\n' "$REGISTRATION_JSON" | jq .
export CONFIRMATION_TOKEN="$(printf '%s' "$REGISTRATION_JSON" | jq -er '.confirmation_token')"
```

Esperado: `201 Created`. Un email duplicado responde `409`.

### `POST /auth/confirm`

Confirma una cuenta y consume el token de un solo uso.

```sh
curl -i -X POST "$BASE_URL/auth/confirm" \
  -H 'Content-Type: application/json' \
  -d "{\"token\":\"$CONFIRMATION_TOKEN\"}"
```

Esperado: `200 OK`. Un token inválido, expirado o reutilizado responde `400`.

### `POST /auth/login`

Sólo permite iniciar sesión después de confirmar la cuenta.

```sh
LOGIN_JSON="$(curl -fsS -X POST "$BASE_URL/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"email\":\"$EMAIL\",\"password\":\"$PASSWORD\"}")"
printf '%s\n' "$LOGIN_JSON" | jq .
export ACCESS_TOKEN="$(printf '%s' "$LOGIN_JSON" | jq -er '.access_token')"
```

Esperado: `200 OK` con `access_token`, `token_type` y `expires_at`.
Credenciales incorrectas o cuenta sin confirmar responden `401`.

### `DELETE /auth/session`

Revoca el token actual.

```sh
curl -i -X DELETE "$BASE_URL/auth/session" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `204 No Content`. Después de ejecutarlo, el mismo token responde
`401` en una ruta protegida. Para seguir probando, vuelve a ejecutar login y
actualiza `ACCESS_TOKEN`.

## 5. Secciones favoritas

Todas estas rutas requieren un Bearer válido.

### `GET /me/favorite-sections`

```sh
curl -i "$BASE_URL/me/favorite-sections" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `200 OK`, por ejemplo `{"items":[]}`.

### `PUT /me/favorite-sections/{key}`

Añade una sección; es idempotente.

```sh
curl -i -X PUT "$BASE_URL/me/favorite-sections/science" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `204 No Content`.

### `DELETE /me/favorite-sections/{key}`

```sh
curl -i -X DELETE "$BASE_URL/me/favorite-sections/science" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `204 No Content`.

## 6. Tags favoritos

Todas estas rutas requieren un Bearer válido.

### `GET /me/favorite-tags`

```sh
curl -i "$BASE_URL/me/favorite-tags" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `200 OK` con `{"items":[...]}`.

### `PUT /me/favorite-tags/{key}`

```sh
curl -i -X PUT "$BASE_URL/me/favorite-tags/technology" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `204 No Content`.

### `DELETE /me/favorite-tags/{key}`

```sh
curl -i -X DELETE "$BASE_URL/me/favorite-tags/technology" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `204 No Content`.

## 7. Actividad

### `POST /activity`

Autenticación opcional. Sin sesión válida requiere `anonymous_id`. Los
`event_type` válidos son `page_view`, `content_view`, `component_click`,
`like` y `comment`. `event_data`, si se envía, debe ser un objeto JSON.

Evento anónimo de visita:

```sh
curl -i -X POST "$BASE_URL/activity" \
  -H 'Content-Type: application/json' \
  -d "{\"anonymous_id\":\"$ANONYMOUS_ID\",\"portal_key\":\"news\",\"event_type\":\"content_view\",\"content_id\":\"note-42\",\"component_id\":\"article\",\"page_url\":\"/notes/42\",\"event_data\":{\"position\":1}}"
```

Evento autenticado de click:

```sh
curl -i -X POST "$BASE_URL/activity" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $ACCESS_TOKEN" \
  -d '{"portal_key":"news","event_type":"component_click","component_id":"hero-cta","page_url":"/"}'
```

Esperado: `201 Created` con `{"event_id":<n>}`. Sin actor responde `400`; un
Bearer inválido responde `401`.

## 8. Likes

### `PUT /content/{id}/like`

Autenticación opcional. `portal_key` es obligatorio.

Like anónimo:

```sh
curl -i -X PUT "$BASE_URL/content/note-42/like" \
  -H 'Content-Type: application/json' \
  -d "{\"anonymous_id\":\"$ANONYMOUS_ID\",\"portal_key\":\"news\"}"
```

Like autenticado:

```sh
curl -i -X PUT "$BASE_URL/content/note-42/like" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $ACCESS_TOKEN" \
  -d '{"portal_key":"news"}'
```

Esperado: `204 No Content`; repetir no duplica el like.

### `DELETE /content/{id}/like`

Para un usuario autenticado no hace falta body:

```sh
curl -i -X DELETE "$BASE_URL/content/note-42/like" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Para una identidad anónima, envía el mismo identificador:

```sh
curl -i -X DELETE "$BASE_URL/content/note-42/like" \
  -H 'Content-Type: application/json' \
  -d "{\"anonymous_id\":\"$ANONYMOUS_ID\"}"
```

Esperado: `204 No Content`.

## 9. Comentarios

### `POST /content/{id}/comments`

Autenticación opcional. Requiere `portal_key` y `body` de 1 a 5000 caracteres;
`author_name` es opcional.

Comentario anónimo:

```sh
curl -i -X POST "$BASE_URL/content/note-42/comments" \
  -H 'Content-Type: application/json' \
  -d "{\"anonymous_id\":\"$ANONYMOUS_ID\",\"portal_key\":\"news\",\"author_name\":\"Invitado\",\"body\":\"Buen artículo\"}"
```

Comentario autenticado:

```sh
curl -i -X POST "$BASE_URL/content/note-42/comments" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $ACCESS_TOKEN" \
  -d '{"portal_key":"news","body":"Comentario de usuario"}'
```

Esperado: `201 Created` con `id` UUID y `created_at`.

## 10. Analítica

### `GET /analytics/top-content`

Requiere autenticación. `portal_key` es obligatorio; `days` acepta 1..365 y
`limit` acepta 1..100. Cuenta `content_view` y `page_view` con `content_id`.

```sh
curl -i "$BASE_URL/analytics/top-content?portal_key=news&days=7&limit=10" \
  -H "Authorization: Bearer $ACCESS_TOKEN"
```

Esperado: `200 OK`.

```json
{
  "portal_key": "news",
  "days": 7,
  "items": [{"content_id":"note-42","views":3}]
}
```

Sin token responde `401`; parámetros fuera de rango responden `400`.

## 11. CORS

### `OPTIONS <cualquier ruta>`

Prueba el preflight para un frontend web:

```sh
curl -i -X OPTIONS "$BASE_URL/activity" \
  -H 'Origin: http://localhost:3000' \
  -H 'Access-Control-Request-Method: POST' \
  -H 'Access-Control-Request-Headers: Authorization, Content-Type'
```

Esperado: `204 No Content` con `Access-Control-Allow-Origin`,
`Access-Control-Allow-Headers` y `Access-Control-Allow-Methods`.

## 12. Errores y prueba automatizada

| Situación | Respuesta |
|---|---|
| JSON vacío o mal formado | `400 invalid_json` |
| Campo obligatorio ausente | `400 validation_error` |
| Bearer ausente en ruta protegida | `401 unauthorized` |
| Bearer inválido, expirado o revocado | `401 invalid_token` o `401 unauthorized` |
| Email ya registrado | `409 email_already_registered` |
| Ruta inexistente | `404 not_found` |
| Body mayor a 1 MiB | `413 payload_too_large` |

Los errores tienen esta forma:

```json
{"error":"validation_error","message":"Descripción del problema"}
```

La prueba automatizada equivalente cubre el flujo principal completo:

```sh
./tests/smoke_api.sh "$BASE_URL"
```

Incluye registro, confirmación, login, favoritos, actividad anónima, like,
comentario, analítica, logout y rechazo de un token revocado.
