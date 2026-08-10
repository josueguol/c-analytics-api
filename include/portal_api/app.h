#ifndef PORTAL_API_APP_H
#define PORTAL_API_APP_H

/**
 * @file app.h
 * @brief Application lifecycle API.
 *
 * PortalApp owns the configuration, database connection, and HTTP server.
 * The type is opaque so callers cannot depend on internal state.
 */

typedef struct PortalApp PortalApp;

/**
 * Creates the application from environment variables.
 *
 * @return An owned application, or NULL when configuration, allocation, or
 * database connection fails. The caller must pass the result to
 * portal_app_destroy().
 */
PortalApp *portal_app_create(void);

/**
 * Starts the HTTP server and waits for shutdown.
 *
 * @return 0 on an orderly shutdown, non-zero if startup or shutdown waiting
 * fails. The caller retains ownership of app.
 */
int portal_app_run(PortalApp *app);

/** Frees the application and sets *app to NULL. Accepts NULL safely. */
void portal_app_destroy(PortalApp **app);

#endif
