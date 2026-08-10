#include "portal_api/http.h"

#include "http_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct PortalHttpServer {
    PortalHttpContext context;
    struct MHD_Daemon *daemon;
};

static enum MHD_Result http_dispatch_content(PortalHttpContext *context,
                                             struct MHD_Connection *connection, const char *url,
                                             const char *method, struct json_object *json) {
    const char *path = url + 9U;
    size_t length = strlen(path);
    size_t suffix_length = 0U;
    char content_id[301U] = {0};

    if ((strcmp(method, MHD_HTTP_METHOD_PUT) == 0 || strcmp(method, MHD_HTTP_METHOD_DELETE) == 0) &&
        length > 5U && strcmp(path + length - 5U, "/like") == 0) {
        suffix_length = 5U;
        if (length - suffix_length >= sizeof(content_id)) {
            return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                              "Invalid content id.");
        }
        (void)memcpy(content_id, path, length - suffix_length);
        content_id[length - suffix_length] = '\0';
        return portal_http_route_like(context, connection, method, content_id, json);
    }
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0 && length > 9U &&
        strcmp(path + length - 9U, "/comments") == 0) {
        suffix_length = 9U;
        if (length - suffix_length >= sizeof(content_id)) {
            return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                              "Invalid content id.");
        }
        (void)memcpy(content_id, path, length - suffix_length);
        content_id[length - suffix_length] = '\0';
        return portal_http_route_comment(context, connection, content_id, json);
    }
    return portal_http_response_error(connection, MHD_HTTP_NOT_FOUND, "not_found",
                                      "Route not found.");
}

static enum MHD_Result http_dispatch_without_body(PortalHttpContext *context,
                                                  struct MHD_Connection *connection,
                                                  const char *url, const char *method,
                                                  bool *handled) {
    static const char health[] = "{\"status\":\"ok\"}";

    *handled = true;
    if (strcmp(method, MHD_HTTP_METHOD_OPTIONS) == 0) {
        return portal_http_response_options(connection);
    }
    if (strcmp(method, MHD_HTTP_METHOD_GET) == 0 && strcmp(url, "/health") == 0) {
        if (!portal_database_is_connected(context->database)) {
            return portal_http_response_error(connection, MHD_HTTP_SERVICE_UNAVAILABLE,
                                              "database_unavailable", "Database is unavailable.");
        }
        return portal_http_response_bytes(connection, MHD_HTTP_OK, health, sizeof(health) - 1U,
                                          "application/json; charset=utf-8");
    }
    if (strcmp(method, MHD_HTTP_METHOD_GET) == 0 && strcmp(url, "/analytics/top-content") == 0) {
        return portal_http_route_top_content(context, connection);
    }
    if (strcmp(method, MHD_HTTP_METHOD_GET) == 0 && strcmp(url, "/me/favorite-sections") == 0) {
        return portal_http_route_favorite_list(context, connection, PORTAL_FAVORITE_SECTION);
    }
    if (strcmp(method, MHD_HTTP_METHOD_GET) == 0 && strcmp(url, "/me/favorite-tags") == 0) {
        return portal_http_route_favorite_list(context, connection, PORTAL_FAVORITE_TAG);
    }
    if (strcmp(method, MHD_HTTP_METHOD_DELETE) == 0 && strcmp(url, "/auth/session") == 0) {
        return portal_http_route_logout(context, connection);
    }
    if (strncmp(url, "/me/favorite-sections/", 22U) == 0 &&
        (strcmp(method, MHD_HTTP_METHOD_PUT) == 0 || strcmp(method, MHD_HTTP_METHOD_DELETE) == 0)) {
        return portal_http_route_favorite_change(context, connection, method,
                                                 PORTAL_FAVORITE_SECTION, url + 22U);
    }
    if (strncmp(url, "/me/favorite-tags/", 18U) == 0 &&
        (strcmp(method, MHD_HTTP_METHOD_PUT) == 0 || strcmp(method, MHD_HTTP_METHOD_DELETE) == 0)) {
        return portal_http_route_favorite_change(context, connection, method, PORTAL_FAVORITE_TAG,
                                                 url + 18U);
    }
    *handled = false;
    return MHD_YES;
}

static enum MHD_Result http_dispatch_json_route(PortalHttpContext *context,
                                                struct MHD_Connection *connection, const char *url,
                                                const char *method, struct json_object *json) {
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0 && strcmp(url, "/auth/register") == 0) {
        return portal_http_route_register(context, connection, json);
    }
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0 && strcmp(url, "/auth/confirm") == 0) {
        return portal_http_route_confirm(context, connection, json);
    }
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0 && strcmp(url, "/auth/login") == 0) {
        return portal_http_route_login(context, connection, json);
    }
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0 && strcmp(url, "/activity") == 0) {
        return portal_http_route_activity(context, connection, json);
    }
    if (strncmp(url, "/content/", 9U) == 0) {
        return http_dispatch_content(context, connection, url, method, json);
    }
    return portal_http_response_error(connection, MHD_HTTP_NOT_FOUND, "not_found",
                                      "Route not found.");
}

static enum MHD_Result http_dispatch(PortalHttpContext *context, struct MHD_Connection *connection,
                                     const char *url, const char *method,
                                     const PortalHttpRequestBody *body) {
    struct json_object *json = NULL;
    enum MHD_Result result = MHD_NO;
    bool handled = false;
    bool body_expected = false;

    result = http_dispatch_without_body(context, connection, url, method, &handled);
    if (handled) {
        return result;
    }
    body_expected = strcmp(method, MHD_HTTP_METHOD_POST) == 0 ||
                    strcmp(method, MHD_HTTP_METHOD_PUT) == 0 ||
                    (strcmp(method, MHD_HTTP_METHOD_DELETE) == 0 && body->length > 0U);
    if (body_expected) {
        json = portal_http_request_parse_json(body);
        if (json == NULL) {
            return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "invalid_json",
                                              "A JSON object request body is required.");
        }
    }
    result = http_dispatch_json_route(context, connection, url, method, json);
    if (json != NULL) {
        json_object_put(json);
        json = NULL;
    }
    return result;
}

static enum MHD_Result http_access_handler(void *cls, struct MHD_Connection *connection,
                                           const char *url, const char *method, const char *version,
                                           const char *upload_data, size_t *upload_data_size,
                                           void **con_cls) {
    PortalHttpContext *context = cls;
    PortalHttpRequestBody *body = *con_cls;
    (void)version;

    if (body == NULL) {
        body = calloc(1U, sizeof(*body));
        if (body == NULL) {
            return MHD_NO;
        }
        *con_cls = body;
        return MHD_YES;
    }
    if (body->response_queued) {
        *upload_data_size = 0U;
        return MHD_YES;
    }
    if (*upload_data_size != 0U) {
        if (!portal_http_request_body_append(body, upload_data, *upload_data_size)) {
            body->response_queued = true;
            *upload_data_size = 0U;
            return portal_http_response_error(
                connection, MHD_HTTP_CONTENT_TOO_LARGE, "payload_too_large",
                "The request body exceeds 1 MiB or could not be allocated.");
        }
        *upload_data_size = 0U;
        return MHD_YES;
    }
    return http_dispatch(context, connection, url, method, body);
}

static void http_request_completed(void *cls, struct MHD_Connection *connection, void **con_cls,
                                   enum MHD_RequestTerminationCode termination_code) {
    PortalHttpRequestBody *body = *con_cls;
    (void)cls;
    (void)connection;
    (void)termination_code;
    portal_http_request_body_destroy(&body);
    *con_cls = NULL;
}

PortalHttpServer *portal_http_server_create(const PortalConfig *config, PortalDatabase *database) {
    PortalHttpServer *server = NULL;
    if (config == NULL || database == NULL) {
        return NULL;
    }
    server = calloc(1U, sizeof(*server));
    if (server == NULL) {
        return NULL;
    }
    server->context.config = config;
    server->context.database = database;
    return server;
}

bool portal_http_server_start(PortalHttpServer *server) {
    if (server == NULL || server->daemon != NULL) {
        return false;
    }
    server->daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD, portal_config_port(server->context.config), NULL, NULL,
        &http_access_handler, &server->context, MHD_OPTION_NOTIFY_COMPLETED,
        &http_request_completed, NULL, MHD_OPTION_END);
    return server->daemon != NULL;
}

void portal_http_server_stop(PortalHttpServer *server) {
    if (server == NULL || server->daemon == NULL) {
        return;
    }
    MHD_stop_daemon(server->daemon);
    server->daemon = NULL;
}

void portal_http_server_destroy(PortalHttpServer **server) {
    if (server == NULL || *server == NULL) {
        return;
    }
    portal_http_server_stop(*server);
    free(*server);
    *server = NULL;
}
