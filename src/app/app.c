#include "portal_api/app.h"

#include "portal_api/config.h"
#include "portal_api/database.h"
#include "portal_api/http.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <conio.h>
#endif

struct PortalApp {
    PortalConfig *config;
    PortalDatabase *database;
    PortalHttpServer *http_server;
};

PortalApp *portal_app_create(void) {
    PortalApp *app = calloc(1U, sizeof(*app));

    if (app == NULL) {
        return NULL;
    }
    app->config = portal_config_create_from_environment();
    if (app->config == NULL) {
        goto cleanup;
    }
    app->database = portal_database_create(portal_config_database_url(app->config));
    if (app->database == NULL) {
        goto cleanup;
    }
    app->http_server = portal_http_server_create(app->config, app->database);
    if (app->http_server == NULL) {
        goto cleanup;
    }
    return app;

cleanup:
    portal_app_destroy(&app);
    return NULL;
}

#if !defined(_WIN32)
static int app_block_shutdown_signals(sigset_t *shutdown_signals, sigset_t *previous_mask) {
    if (sigemptyset(shutdown_signals) != 0 || sigaddset(shutdown_signals, SIGINT) != 0 ||
        sigaddset(shutdown_signals, SIGTERM) != 0) {
        return -1;
    }
    return sigprocmask(SIG_BLOCK, shutdown_signals, previous_mask);
}
#endif

int portal_app_run(PortalApp *app) {
    int result = 1;

    if (app == NULL || app->http_server == NULL) {
        return result;
    }
#if defined(_WIN32)
    if (!portal_http_server_start(app->http_server)) {
        return result;
    }
    (void)fprintf(stderr, "portal-api listening on port %u; press a key to stop\n",
                  portal_config_port(app->config));
    (void)_getch();
    result = 0;
#else
    sigset_t shutdown_signals;
    sigset_t previous_mask;
    int received_signal = 0;

    if (app_block_shutdown_signals(&shutdown_signals, &previous_mask) != 0) {
        return result;
    }
    if (!portal_http_server_start(app->http_server)) {
        goto cleanup;
    }
    (void)fprintf(stderr, "portal-api listening on port %u\n", portal_config_port(app->config));
    if (sigwait(&shutdown_signals, &received_signal) == 0) {
        result = 0;
    }

cleanup:
    portal_http_server_stop(app->http_server);
    if (sigprocmask(SIG_SETMASK, &previous_mask, NULL) != 0) {
        result = 1;
    }
#endif
    return result;
}

void portal_app_destroy(PortalApp **app) {
    if (app == NULL || *app == NULL) {
        return;
    }
    portal_http_server_destroy(&(*app)->http_server);
    portal_database_destroy(&(*app)->database);
    portal_config_destroy(&(*app)->config);
    free(*app);
    *app = NULL;
}
