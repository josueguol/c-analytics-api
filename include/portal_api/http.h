#ifndef PORTAL_API_HTTP_H
#define PORTAL_API_HTTP_H

#include <stdbool.h>

typedef struct PortalConfig PortalConfig;
typedef struct PortalDatabase PortalDatabase;
typedef struct PortalHttpServer PortalHttpServer;

/**
 * Allocates an HTTP server. config and database are borrowed and must outlive
 * the server. Returns NULL on allocation failure.
 */
PortalHttpServer *portal_http_server_create(const PortalConfig *config, PortalDatabase *database);

/** Starts the server. Returns false if it is already running or cannot start. */
bool portal_http_server_start(PortalHttpServer *server);

/** Stops a running server. Safe to call when already stopped. */
void portal_http_server_stop(PortalHttpServer *server);

/** Stops, frees, and sets *server to NULL. Accepts NULL safely. */
void portal_http_server_destroy(PortalHttpServer **server);

#endif
