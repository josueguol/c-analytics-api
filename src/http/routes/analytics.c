#include "../http_internal.h"

#include "portal_api/validation.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static bool analytics_parse_range(const char *text, long fallback, long minimum, long maximum,
                                  long *output) {
    char *end = NULL;
    long value = fallback;

    if (output == NULL) {
        return false;
    }
    if (text != NULL) {
        errno = 0;
        value = strtol(text, &end, 10);
        if (errno != 0 || end == text || *end != '\0' || value < minimum || value > maximum) {
            return false;
        }
    }
    *output = value;
    return true;
}

enum MHD_Result portal_http_route_top_content(PortalHttpContext *context,
                                              struct MHD_Connection *connection) {
    PortalAuthenticatedUser user = {0};
    const char *portal =
        MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "portal_key");
    const char *days_text = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "days");
    const char *limit_text =
        MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "limit");
    const char *values[3] = {NULL};
    char days_buffer[12U] = {0};
    char limit_buffer[12U] = {0};
    long days = 7L;
    long limit = 10L;
    PortalDatabaseResult *database_result = NULL;
    struct json_object *response = NULL;
    struct json_object *items = NULL;
    enum MHD_Result result = MHD_NO;
    size_t index = 0U;
    static const char query[] =
        "SELECT content_id, count(*)::text FROM activity_events "
        "WHERE portal_key=$1 AND content_id IS NOT NULL "
        "AND event_type IN ('content_view', 'page_view') "
        "AND occurred_at >= now() - ($2::int * interval '1 day') "
        "GROUP BY content_id ORDER BY count(*) DESC, content_id ASC LIMIT $3::int";

    if (portal_http_authenticate(context, connection, &user) != PORTAL_AUTH_AUTHENTICATED) {
        return portal_http_response_error(connection, MHD_HTTP_UNAUTHORIZED, "unauthorized",
                                          "A valid Bearer token is required.");
    }
    if (portal == NULL || !portal_validation_key(portal, 120U)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "portal_key is required.");
    }
    if (!analytics_parse_range(days_text, 7L, 1L, 365L, &days) ||
        !analytics_parse_range(limit_text, 10L, 1L, 100L, &limit)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "days must be 1..365 and limit must be 1..100.");
    }
    if (snprintf(days_buffer, sizeof(days_buffer), "%ld", days) < 0 ||
        snprintf(limit_buffer, sizeof(limit_buffer), "%ld", limit) < 0) {
        return portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                          "serialization_error",
                                          "Could not prepare analytics query.");
    }
    values[0] = portal;
    values[1] = days_buffer;
    values[2] = limit_buffer;
    database_result = portal_database_query(context->database, query, 3U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES) {
        result = portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                            "database_error", "Could not load analytics.");
        goto cleanup;
    }
    response = json_object_new_object();
    items = json_object_new_array();
    if (response == NULL || items == NULL) {
        result = MHD_NO;
        goto cleanup;
    }
    for (index = 0U; index < portal_database_result_row_count(database_result); ++index) {
        struct json_object *item = json_object_new_object();
        if (item == NULL) {
            result = MHD_NO;
            goto cleanup;
        }
        json_object_object_add(
            item, "content_id",
            json_object_new_string(portal_database_result_value(database_result, index, 0U)));
        json_object_object_add(
            item, "views",
            json_object_new_int64(
                strtoll(portal_database_result_value(database_result, index, 1U), NULL, 10)));
        json_object_array_add(items, item);
    }
    json_object_object_add(response, "portal_key", json_object_new_string(portal));
    json_object_object_add(response, "days", json_object_new_int64(days));
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
