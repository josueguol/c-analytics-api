#include "http_internal.h"

#include <string.h>

enum MHD_Result portal_http_response_bytes(struct MHD_Connection *connection, unsigned int status,
                                           const char *data, size_t length,
                                           const char *content_type) {
    struct MHD_Response *response = NULL;
    enum MHD_Result result = MHD_NO;

    if (connection == NULL || data == NULL || content_type == NULL) {
        return MHD_NO;
    }
    response = MHD_create_response_from_buffer(length, (void *)data, MHD_RESPMEM_MUST_COPY);
    if (response == NULL) {
        return MHD_NO;
    }
    if (MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, content_type) == MHD_NO ||
        MHD_add_response_header(response, "X-Content-Type-Options", "nosniff") == MHD_NO ||
        MHD_add_response_header(response, "Cache-Control", "no-store") == MHD_NO ||
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*") ==
            MHD_NO) {
        goto cleanup;
    }
    result = MHD_queue_response(connection, status, response);

cleanup:
    MHD_destroy_response(response);
    response = NULL;
    return result;
}

enum MHD_Result portal_http_response_json(struct MHD_Connection *connection, unsigned int status,
                                          struct json_object *json) {
    const char *text = NULL;
    if (json == NULL) {
        return MHD_NO;
    }
    text = json_object_to_json_string_ext(json, JSON_C_TO_STRING_PLAIN);
    return portal_http_response_bytes(connection, status, text, strlen(text),
                                      "application/json; charset=utf-8");
}

enum MHD_Result portal_http_response_empty(struct MHD_Connection *connection, unsigned int status) {
    return portal_http_response_bytes(connection, status, "", 0U,
                                      "application/json; charset=utf-8");
}

enum MHD_Result portal_http_response_error(struct MHD_Connection *connection, unsigned int status,
                                           const char *code, const char *message) {
    struct json_object *json = NULL;
    enum MHD_Result result = MHD_NO;

    if (code == NULL || message == NULL) {
        return MHD_NO;
    }
    json = json_object_new_object();
    if (json == NULL) {
        return MHD_NO;
    }
    json_object_object_add(json, "error", json_object_new_string(code));
    json_object_object_add(json, "message", json_object_new_string(message));
    result = portal_http_response_json(connection, status, json);
    json_object_put(json);
    json = NULL;
    return result;
}

enum MHD_Result portal_http_response_options(struct MHD_Connection *connection) {
    struct MHD_Response *response = NULL;
    enum MHD_Result result = MHD_NO;

    response = MHD_create_response_from_buffer(0U, "", MHD_RESPMEM_PERSISTENT);
    if (response == NULL) {
        return MHD_NO;
    }
    if (MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*") ==
            MHD_NO ||
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_HEADERS,
                                "Authorization, Content-Type") == MHD_NO ||
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_METHODS,
                                "GET, POST, PUT, DELETE, OPTIONS") == MHD_NO) {
        goto cleanup;
    }
    result = MHD_queue_response(connection, MHD_HTTP_NO_CONTENT, response);

cleanup:
    MHD_destroy_response(response);
    response = NULL;
    return result;
}
