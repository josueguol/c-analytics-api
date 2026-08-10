# Arquitectura

La aplicación se divide por responsabilidad y todas las dependencias entre
módulos pasan por interfaces opacas ubicadas en `include/portal_api/`.

## Módulos

| Módulo | Responsabilidad | Ownership y errores |
|---|---|---|
| `app` | Crear y coordinar el ciclo de vida completo. | Es dueño de configuración, base de datos y servidor HTTP. Libera en orden inverso. |
| `config` | Leer y validar variables de entorno. | Es dueño de las copias de strings. Devuelve `NULL` si falta `DATABASE_URL` o falla memoria. |
| `database` | Encapsular libpq y resultados SQL parametrizados. | Cada `PortalDatabaseResult` pertenece al llamador y debe destruirse. Los errores SQL se conservan en el resultado. |
| `http` | Gestionar libmicrohttpd, request bodies, respuestas y routing. | El callback de finalización es dueño del body. El servidor toma prestados configuración y conexión. |
| `http/routes` | Implementar autenticación, favoritos, actividad y analítica. | JSON y parámetros son prestados durante la petición; cada resultado de BD se libera en un único bloque de cleanup. |
| `validation` | Validaciones puras reutilizables. | No reserva memoria y no conserva referencias. |

## Dependencias

```text
main -> app -> config
            -> database -> libpq
            -> http -> libmicrohttpd
                    -> json-c
                    -> validation
                    -> database
```

Las estructuras de `PortalApp`, `PortalConfig`, `PortalDatabase` y
`PortalHttpServer` permanecen privadas. La API pública sólo ofrece handles
opacos con prefijo `portal_`.

## Memoria y concurrencia

El servidor usa el polling thread interno de libmicrohttpd. Las consultas se
ejecutan en ese mismo hilo sobre una única conexión PostgreSQL, por lo que no se
comparte `PGconn` concurrentemente. Esta decisión es adecuada para la fase
inicial; antes de habilitar múltiples worker threads se deberá introducir un
pool de conexiones explícito.

Los request bodies están limitados a 1 MiB y se liberan desde el callback de
terminación, incluso si la petición se corta o se rechaza anticipadamente.
