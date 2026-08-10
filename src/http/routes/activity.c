#include "../http_internal.h"

#include "portal_api/validation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    PortalAuthStatus status;
    PortalAuthenticatedUser user;
    const char *anonymous_id;
    char actor_key[205U];
} PortalActivityActor;

static PortalAuthStatus activity_load_actor(PortalHttpContext *context,
                                            struct MHD_Connection *connection,
                                            struct json_object *json, PortalActivityActor *actor) {
    int written = 0;

    actor->status = portal_http_authenticate(context, connection, &actor->user);
    actor->anonymous_id = portal_http_json_optional_string(json, "anonymous_id", 200U);
    if (actor->status == PORTAL_AUTH_INVALID) {
        return PORTAL_AUTH_INVALID;
    }
    if (actor->status == PORTAL_AUTH_AUTHENTICATED) {
        written = snprintf(actor->actor_key, sizeof(actor->actor_key), "u:%s", actor->user.id);
    } else {
        if (!portal_validation_anonymous_id(actor->anonymous_id)) {
            return PORTAL_AUTH_INVALID;
        }
        written = snprintf(actor->actor_key, sizeof(actor->actor_key), "a:%s", actor->anonymous_id);
    }
    if (written < 0 || (size_t)written >= sizeof(actor->actor_key)) {
        return PORTAL_AUTH_INVALID;
    }
    return actor->status;
}

static enum MHD_Result activity_actor_error(struct MHD_Connection *connection,
                                            PortalAuthStatus authentication) {
    if (authentication == PORTAL_AUTH_INVALID) {
        const char *authorization =
            MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);
        if (authorization != NULL && *authorization != '\0') {
            return portal_http_response_error(connection, MHD_HTTP_UNAUTHORIZED, "invalid_token",
                                              "The supplied Bearer token is invalid.");
        }
    }
    return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "anonymous_id_required",
                                      "Anonymous actions need an opaque anonymous_id.");
}

enum MHD_Result portal_http_route_activity(PortalHttpContext *context,
                                           struct MHD_Connection *connection,
                                           struct json_object *json) {
    PortalActivityActor actor = {0};
    const char *portal = portal_http_json_string(json, "portal_key", 1U, 120U);
    const char *event_type = portal_http_json_string(json, "event_type", 1U, 40U);
    const char *content_id = portal_http_json_optional_string(json, "content_id", 300U);
    const char *component_id = portal_http_json_optional_string(json, "component_id", 300U);
    const char *page_url = portal_http_json_optional_string(json, "page_url", 2000U);
    const char *data_text = "{}";
    const char *values[8] = {NULL};
    struct json_object *event_data = NULL;
    struct json_object *response = NULL;
    PortalDatabaseResult *database_result = NULL;
    enum MHD_Result result = MHD_NO;
    PortalAuthStatus authentication = PORTAL_AUTH_INVALID;
    static const char query[] =
        "INSERT INTO activity_events "
        "(user_id, anonymous_id, portal_key, event_type, content_id, component_id, "
        "page_url, event_data) "
        "VALUES (NULLIF($1, '')::uuid, NULLIF($2, ''), $3, $4, NULLIF($5, ''), "
        "NULLIF($6, ''), NULLIF($7, ''), $8::jsonb) RETURNING id";

    if (portal == NULL || event_type == NULL || !portal_validation_key(portal, 120U) ||
        !portal_validation_event_type(event_type)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "portal_key and a supported event_type are required.");
    }
    authentication = activity_load_actor(context, connection, json, &actor);
    if (authentication == PORTAL_AUTH_INVALID) {
        return activity_actor_error(connection, authentication);
    }
    if (json_object_object_get_ex(json, "event_data", &event_data)) {
        if (!json_object_is_type(event_data, json_type_object)) {
            return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                              "event_data must be a JSON object.");
        }
        data_text = json_object_to_json_string_ext(event_data, JSON_C_TO_STRING_PLAIN);
    }
    values[0] = authentication == PORTAL_AUTH_AUTHENTICATED ? actor.user.id : "";
    values[1] = authentication == PORTAL_AUTH_AUTHENTICATED ? "" : actor.anonymous_id;
    values[2] = portal;
    values[3] = event_type;
    values[4] = content_id == NULL ? "" : content_id;
    values[5] = component_id == NULL ? "" : component_id;
    values[6] = page_url == NULL ? "" : page_url;
    values[7] = data_text;
    database_result = portal_database_query(context->database, query, 8U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES) {
        result = portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                            "database_error", "Could not record activity.");
        goto cleanup;
    }
    response = json_object_new_object();
    if (response == NULL) {
        result = MHD_NO;
        goto cleanup;
    }
    json_object_object_add(response, "event_id",
                           json_object_new_int64(strtoll(
                               portal_database_result_value(database_result, 0U, 0U), NULL, 10)));
    result = portal_http_response_json(connection, MHD_HTTP_CREATED, response);

cleanup:
    if (response != NULL) {
        json_object_put(response);
        response = NULL;
    }
    portal_database_result_destroy(&database_result);
    return result;
}

enum MHD_Result portal_http_route_like(PortalHttpContext *context,
                                       struct MHD_Connection *connection, const char *method,
                                       const char *content_id, struct json_object *json) {
    PortalActivityActor actor = {0};
    PortalAuthStatus authentication = PORTAL_AUTH_INVALID;
    const char *portal = portal_http_json_string(json, "portal_key", 1U, 120U);
    const char *values[5] = {NULL};
    PortalDatabaseResult *database_result = NULL;
    enum MHD_Result result = MHD_NO;
    bool is_put = strcmp(method, MHD_HTTP_METHOD_PUT) == 0;
    static const char insert_query[] =
        "WITH added AS ("
        " INSERT INTO content_likes (content_id, user_id, anonymous_id, actor_key) "
        " VALUES ($1, NULLIF($2, '')::uuid, NULLIF($3, ''), $4) "
        " ON CONFLICT DO NOTHING RETURNING 1"
        ") INSERT INTO activity_events "
        "(user_id, anonymous_id, portal_key, event_type, content_id) "
        "SELECT NULLIF($2, '')::uuid, NULLIF($3, ''), $5, 'like', $1 FROM added "
        "RETURNING id";

    if (!portal_validation_key(content_id, 300U)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "The content id contains unsupported characters.");
    }
    authentication = activity_load_actor(context, connection, json, &actor);
    if (authentication == PORTAL_AUTH_INVALID) {
        return activity_actor_error(connection, authentication);
    }
    if (is_put && (portal == NULL || !portal_validation_key(portal, 120U))) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "portal_key is required for a like.");
    }
    values[0] = content_id;
    values[1] = authentication == PORTAL_AUTH_AUTHENTICATED ? actor.user.id : "";
    values[2] = authentication == PORTAL_AUTH_AUTHENTICATED ? "" : actor.anonymous_id;
    values[3] = actor.actor_key;
    values[4] = portal == NULL ? "" : portal;
    if (is_put) {
        database_result = portal_database_query(context->database, insert_query, 5U, values);
    } else {
        const char *delete_values[2] = {content_id, actor.actor_key};
        database_result = portal_database_query(
            context->database, "DELETE FROM content_likes WHERE content_id=$1 AND actor_key=$2", 2U,
            delete_values);
    }
    if ((is_put &&
         portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES) ||
        (!is_put &&
         portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_COMMAND)) {
        result = portal_http_response_error(
            connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "database_error",
            is_put ? "Could not store the like." : "Could not remove the like.");
        goto cleanup;
    }
    result = portal_http_response_empty(connection, MHD_HTTP_NO_CONTENT);

cleanup:
    portal_database_result_destroy(&database_result);
    return result;
}

enum MHD_Result portal_http_route_comment(PortalHttpContext *context,
                                          struct MHD_Connection *connection, const char *content_id,
                                          struct json_object *json) {
    PortalActivityActor actor = {0};
    PortalAuthStatus authentication = PORTAL_AUTH_INVALID;
    const char *body = portal_http_json_string(json, "body", 1U, 5000U);
    const char *author_name = portal_http_json_optional_string(json, "author_name", 100U);
    const char *portal = portal_http_json_string(json, "portal_key", 1U, 120U);
    const char *values[6] = {NULL};
    PortalDatabaseResult *database_result = NULL;
    struct json_object *response = NULL;
    enum MHD_Result result = MHD_NO;
    static const char query[] =
        "WITH created_comment AS ("
        " INSERT INTO comments (content_id, user_id, anonymous_id, author_name, body) "
        " VALUES ($1, NULLIF($2, '')::uuid, NULLIF($3, ''), NULLIF($4, ''), $5) "
        " RETURNING id, created_at"
        "), created_event AS ("
        " INSERT INTO activity_events "
        " (user_id, anonymous_id, portal_key, event_type, content_id) "
        " SELECT NULLIF($2, '')::uuid, NULLIF($3, ''), $6, 'comment', $1 "
        " FROM created_comment"
        ") SELECT id::text, created_at::text FROM created_comment";

    if (!portal_validation_key(content_id, 300U) || portal == NULL ||
        !portal_validation_key(portal, 120U) || body == NULL) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "content id, portal_key and comment body are required.");
    }
    authentication = activity_load_actor(context, connection, json, &actor);
    if (authentication == PORTAL_AUTH_INVALID) {
        return activity_actor_error(connection, authentication);
    }
    values[0] = content_id;
    values[1] = authentication == PORTAL_AUTH_AUTHENTICATED ? actor.user.id : "";
    values[2] = authentication == PORTAL_AUTH_AUTHENTICATED ? "" : actor.anonymous_id;
    values[3] = author_name == NULL ? "" : author_name;
    values[4] = body;
    values[5] = portal;
    database_result = portal_database_query(context->database, query, 6U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES ||
        portal_database_result_row_count(database_result) != 1U) {
        result = portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                            "database_error", "Could not store the comment.");
        goto cleanup;
    }
    response = json_object_new_object();
    if (response == NULL) {
        result = MHD_NO;
        goto cleanup;
    }
    json_object_object_add(
        response, "id",
        json_object_new_string(portal_database_result_value(database_result, 0U, 0U)));
    json_object_object_add(
        response, "created_at",
        json_object_new_string(portal_database_result_value(database_result, 0U, 1U)));
    result = portal_http_response_json(connection, MHD_HTTP_CREATED, response);

cleanup:
    if (response != NULL) {
        json_object_put(response);
        response = NULL;
    }
    portal_database_result_destroy(&database_result);
    return result;
}
