#ifndef PORTAL_API_HTTP_INTERNAL_H
#define PORTAL_API_HTTP_INTERNAL_H

#include "portal_api/config.h"
#include "portal_api/database.h"

#include <json-c/json.h>
#include <microhttpd.h>
#include <stdbool.h>
#include <stddef.h>

#define PORTAL_HTTP_MAX_BODY_SIZE ((size_t)1024U * 1024U)
#define PORTAL_HTTP_UUID_TEXT_LENGTH 36U

typedef struct {
    const PortalConfig *config;
    PortalDatabase *database;
} PortalHttpContext;

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    bool response_queued;
} PortalHttpRequestBody;

typedef struct {
    char id[PORTAL_HTTP_UUID_TEXT_LENGTH + 1U];
    char email[256U];
} PortalAuthenticatedUser;

typedef enum {
    PORTAL_AUTH_INVALID = -1,
    PORTAL_AUTH_ANONYMOUS = 0,
    PORTAL_AUTH_AUTHENTICATED = 1
} PortalAuthStatus;

typedef enum { PORTAL_FAVORITE_SECTION = 0, PORTAL_FAVORITE_TAG = 1 } PortalFavoriteKind;

enum MHD_Result portal_http_response_bytes(struct MHD_Connection *connection, unsigned int status,
                                           const char *data, size_t length,
                                           const char *content_type);
enum MHD_Result portal_http_response_json(struct MHD_Connection *connection, unsigned int status,
                                          struct json_object *json);
enum MHD_Result portal_http_response_empty(struct MHD_Connection *connection, unsigned int status);
enum MHD_Result portal_http_response_error(struct MHD_Connection *connection, unsigned int status,
                                           const char *code, const char *message);
enum MHD_Result portal_http_response_options(struct MHD_Connection *connection);

bool portal_http_request_body_append(PortalHttpRequestBody *body, const char *data, size_t size);
void portal_http_request_body_destroy(PortalHttpRequestBody **body);
struct json_object *portal_http_request_parse_json(const PortalHttpRequestBody *body);
const char *portal_http_json_string(struct json_object *json, const char *key,
                                    size_t minimum_length, size_t maximum_length);
const char *portal_http_json_optional_string(struct json_object *json, const char *key,
                                             size_t maximum_length);

PortalAuthStatus portal_http_authenticate(PortalHttpContext *context,
                                          struct MHD_Connection *connection,
                                          PortalAuthenticatedUser *user);

enum MHD_Result portal_http_route_register(PortalHttpContext *context,
                                           struct MHD_Connection *connection,
                                           struct json_object *json);
enum MHD_Result portal_http_route_confirm(PortalHttpContext *context,
                                          struct MHD_Connection *connection,
                                          struct json_object *json);
enum MHD_Result portal_http_route_login(PortalHttpContext *context,
                                        struct MHD_Connection *connection,
                                        struct json_object *json);
enum MHD_Result portal_http_route_logout(PortalHttpContext *context,
                                         struct MHD_Connection *connection);
enum MHD_Result portal_http_route_favorite_list(PortalHttpContext *context,
                                                struct MHD_Connection *connection,
                                                PortalFavoriteKind kind);
enum MHD_Result portal_http_route_favorite_change(PortalHttpContext *context,
                                                  struct MHD_Connection *connection,
                                                  const char *method, PortalFavoriteKind kind,
                                                  const char *key);
enum MHD_Result portal_http_route_activity(PortalHttpContext *context,
                                           struct MHD_Connection *connection,
                                           struct json_object *json);
enum MHD_Result portal_http_route_like(PortalHttpContext *context,
                                       struct MHD_Connection *connection, const char *method,
                                       const char *content_id, struct json_object *json);
enum MHD_Result portal_http_route_comment(PortalHttpContext *context,
                                          struct MHD_Connection *connection, const char *content_id,
                                          struct json_object *json);
enum MHD_Result portal_http_route_top_content(PortalHttpContext *context,
                                              struct MHD_Connection *connection);

#endif
