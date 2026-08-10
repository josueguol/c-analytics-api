#include "../http_internal.h"

#include "portal_api/validation.h"

#include <json-c/json.h>
#include <string.h>

static bool favorite_has_authenticated_user(PortalHttpContext *context,
                                            struct MHD_Connection *connection,
                                            PortalAuthenticatedUser *user) {
    return portal_http_authenticate(context, connection, user) == PORTAL_AUTH_AUTHENTICATED;
}

enum MHD_Result portal_http_route_favorite_list(PortalHttpContext *context,
                                                struct MHD_Connection *connection,
                                                PortalFavoriteKind kind) {
    PortalAuthenticatedUser user = {{0}, {0}};
    const char *values[1] = {NULL};
    const char *query = kind == PORTAL_FAVORITE_TAG ? "SELECT tag_key FROM favorite_tags "
                                                      "WHERE user_id=$1::uuid ORDER BY tag_key"
                                                    : "SELECT section_key FROM favorite_sections "
                                                      "WHERE user_id=$1::uuid ORDER BY section_key";
    PortalDatabaseResult *database_result = NULL;
    struct json_object *response = NULL;
    struct json_object *items = NULL;
    enum MHD_Result result = MHD_NO;
    size_t index = 0U;

    if (!favorite_has_authenticated_user(context, connection, &user)) {
        return portal_http_response_error(connection, MHD_HTTP_UNAUTHORIZED, "unauthorized",
                                          "A valid Bearer token is required.");
    }
    values[0] = user.id;
    database_result = portal_database_query(context->database, query, 1U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES) {
        result = portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                            "database_error", "Could not load favorites.");
        goto cleanup;
    }
    response = json_object_new_object();
    items = json_object_new_array();
    if (response == NULL || items == NULL) {
        result = MHD_NO;
        goto cleanup;
    }
    for (index = 0U; index < portal_database_result_row_count(database_result); ++index) {
        json_object_array_add(items, json_object_new_string(
                                         portal_database_result_value(database_result, index, 0U)));
    }
    json_object_object_add(response, "items", items);
    items = NULL;
    result = portal_http_response_json(connection, MHD_HTTP_OK, response);

cleanup:
    if (items != NULL) {
        json_object_put(items);
        items = NULL;
    }
    if (response != NULL) {
        json_object_put(response);
        response = NULL;
    }
    portal_database_result_destroy(&database_result);
    return result;
}

enum MHD_Result portal_http_route_favorite_change(PortalHttpContext *context,
                                                  struct MHD_Connection *connection,
                                                  const char *method, PortalFavoriteKind kind,
                                                  const char *key) {
    PortalAuthenticatedUser user = {{0}, {0}};
    const char *values[2] = {NULL};
    const char *query = NULL;
    PortalDatabaseResult *database_result = NULL;
    enum MHD_Result result = MHD_NO;
    bool is_put = strcmp(method, MHD_HTTP_METHOD_PUT) == 0;

    if (!favorite_has_authenticated_user(context, connection, &user)) {
        return portal_http_response_error(connection, MHD_HTTP_UNAUTHORIZED, "unauthorized",
                                          "A valid Bearer token is required.");
    }
    if (!portal_validation_key(key, 160U)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "The favorite key contains unsupported characters.");
    }
    if (kind == PORTAL_FAVORITE_TAG) {
        query = is_put ? "INSERT INTO favorite_tags (user_id, tag_key) "
                         "VALUES ($1::uuid, $2) ON CONFLICT DO NOTHING"
                       : "DELETE FROM favorite_tags "
                         "WHERE user_id=$1::uuid AND tag_key=$2";
    } else {
        query = is_put ? "INSERT INTO favorite_sections (user_id, section_key) "
                         "VALUES ($1::uuid, $2) ON CONFLICT DO NOTHING"
                       : "DELETE FROM favorite_sections "
                         "WHERE user_id=$1::uuid AND section_key=$2";
    }
    values[0] = user.id;
    values[1] = key;
    database_result = portal_database_query(context->database, query, 2U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_COMMAND) {
        result = portal_http_response_error(
            connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "database_error",
            is_put ? "Could not save the favorite." : "Could not remove the favorite.");
        goto cleanup;
    }
    result = portal_http_response_empty(connection, MHD_HTTP_NO_CONTENT);

cleanup:
    portal_database_result_destroy(&database_result);
    return result;
}
