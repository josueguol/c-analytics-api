#include "../http_internal.h"

#include "portal_api/validation.h"

#include <stdio.h>
#include <string.h>

static enum MHD_Result auth_database_error(struct MHD_Connection *connection, const char *message) {
    return portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "database_error",
                                      message);
}

enum MHD_Result portal_http_route_register(PortalHttpContext *context,
                                           struct MHD_Connection *connection,
                                           struct json_object *json) {
    const char *email = portal_http_json_string(json, "email", 5U, 254U);
    const char *password = portal_http_json_string(json, "password", 12U, 256U);
    const char *display_name = portal_http_json_string(json, "display_name", 1U, 100U);
    const char *values[4] = {NULL};
    char normalized_email[255U] = {0};
    char ttl[12U] = {0};
    PortalDatabaseResult *database_result = NULL;
    struct json_object *response = NULL;
    enum MHD_Result result = MHD_NO;
    int written = 0;
    static const char query[] = "WITH new_user AS ("
                                " INSERT INTO users (email, display_name, password_hash) "
                                " VALUES ($1, $3, crypt($2, gen_salt('bf', 12))) RETURNING id"
                                "), new_token AS ("
                                " INSERT INTO confirmation_tokens (user_id, expires_at) "
                                " SELECT id, now() + ($4::int * interval '1 hour') FROM new_user "
                                " RETURNING token, expires_at"
                                ") SELECT token::text, expires_at::text FROM new_token";

    if (email == NULL || password == NULL || display_name == NULL ||
        !portal_validation_email(email)) {
        return portal_http_response_error(
            connection, MHD_HTTP_BAD_REQUEST, "validation_error",
            "email, display_name and a password of at least 12 characters are required.");
    }
    if (!portal_validation_lowercase_copy(normalized_email, sizeof(normalized_email), email)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "Invalid email.");
    }
    written =
        snprintf(ttl, sizeof(ttl), "%u", portal_config_confirmation_ttl_hours(context->config));
    if (written < 0 || (size_t)written >= sizeof(ttl)) {
        return portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                          "configuration_error", "Invalid token lifetime.");
    }
    values[0] = normalized_email;
    values[1] = password;
    values[2] = display_name;
    values[3] = ttl;
    database_result = portal_database_query(context->database, query, 4U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES) {
        bool duplicate = portal_database_result_is_unique_violation(database_result);
        result = portal_http_response_error(
            connection, duplicate ? MHD_HTTP_CONFLICT : MHD_HTTP_INTERNAL_SERVER_ERROR,
            duplicate ? "email_already_registered" : "database_error",
            duplicate ? "An account with this email already exists."
                      : "Could not create the account.");
        goto cleanup;
    }
    response = json_object_new_object();
    if (response == NULL) {
        result = MHD_NO;
        goto cleanup;
    }
    json_object_object_add(
        response, "message",
        json_object_new_string("Account created. Confirm it before signing in."));
    json_object_object_add(
        response, "confirmation_expires_at",
        json_object_new_string(portal_database_result_value(database_result, 0U, 1U)));
    if (portal_config_exposes_confirmation_token(context->config)) {
        json_object_object_add(
            response, "confirmation_token",
            json_object_new_string(portal_database_result_value(database_result, 0U, 0U)));
    }
    result = portal_http_response_json(connection, MHD_HTTP_CREATED, response);

cleanup:
    if (response != NULL) {
        json_object_put(response);
        response = NULL;
    }
    portal_database_result_destroy(&database_result);
    return result;
}

enum MHD_Result portal_http_route_confirm(PortalHttpContext *context,
                                          struct MHD_Connection *connection,
                                          struct json_object *json) {
    const char *token = portal_http_json_string(json, "token", PORTAL_HTTP_UUID_TEXT_LENGTH,
                                                PORTAL_HTTP_UUID_TEXT_LENGTH);
    const char *values[1] = {token};
    PortalDatabaseResult *database_result = NULL;
    enum MHD_Result result = MHD_NO;
    static const char query[] =
        "WITH consumed AS ("
        " UPDATE confirmation_tokens SET consumed_at=now() "
        " WHERE token=$1::uuid AND consumed_at IS NULL AND expires_at > now() "
        " RETURNING user_id"
        ") UPDATE users u "
        "SET confirmed_at=COALESCE(u.confirmed_at, now()), updated_at=now() "
        "FROM consumed c WHERE u.id=c.user_id RETURNING u.id";
    static const char success[] = "{\"message\":\"Account confirmed. You can now sign in.\"}";

    if (token == NULL) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "A confirmation token is required.");
    }
    database_result = portal_database_query(context->database, query, 1U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES ||
        portal_database_result_row_count(database_result) != 1U) {
        result = portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST,
                                            "invalid_confirmation_token",
                                            "The confirmation token is invalid or expired.");
        goto cleanup;
    }
    result = portal_http_response_bytes(connection, MHD_HTTP_OK, success, sizeof(success) - 1U,
                                        "application/json; charset=utf-8");

cleanup:
    portal_database_result_destroy(&database_result);
    return result;
}

enum MHD_Result portal_http_route_login(PortalHttpContext *context,
                                        struct MHD_Connection *connection,
                                        struct json_object *json) {
    const char *email = portal_http_json_string(json, "email", 5U, 254U);
    const char *password = portal_http_json_string(json, "password", 1U, 256U);
    const char *values[3] = {NULL};
    char normalized_email[255U] = {0};
    char ttl[12U] = {0};
    PortalDatabaseResult *database_result = NULL;
    struct json_object *response = NULL;
    enum MHD_Result result = MHD_NO;
    int written = 0;
    static const char query[] = "WITH candidate AS ("
                                " SELECT id FROM users WHERE email=$1 AND confirmed_at IS NOT NULL "
                                " AND password_hash=crypt($2, password_hash)"
                                "), session AS ("
                                " INSERT INTO auth_sessions (user_id, expires_at) "
                                " SELECT id, now() + ($3::int * interval '1 hour') FROM candidate "
                                " RETURNING token, expires_at"
                                ") SELECT token::text, expires_at::text FROM session";

    if (email == NULL || password == NULL || !portal_validation_email(email)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "email and password are required.");
    }
    if (!portal_validation_lowercase_copy(normalized_email, sizeof(normalized_email), email)) {
        return portal_http_response_error(connection, MHD_HTTP_BAD_REQUEST, "validation_error",
                                          "Invalid email.");
    }
    written = snprintf(ttl, sizeof(ttl), "%u", portal_config_token_ttl_hours(context->config));
    if (written < 0 || (size_t)written >= sizeof(ttl)) {
        return portal_http_response_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                          "configuration_error", "Invalid token lifetime.");
    }
    values[0] = normalized_email;
    values[1] = password;
    values[2] = ttl;
    database_result = portal_database_query(context->database, query, 3U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_TUPLES) {
        result = auth_database_error(connection, "Could not sign in.");
        goto cleanup;
    }
    if (portal_database_result_row_count(database_result) != 1U) {
        result =
            portal_http_response_error(connection, MHD_HTTP_UNAUTHORIZED, "invalid_credentials",
                                       "Invalid credentials or unconfirmed account.");
        goto cleanup;
    }
    response = json_object_new_object();
    if (response == NULL) {
        result = MHD_NO;
        goto cleanup;
    }
    json_object_object_add(
        response, "access_token",
        json_object_new_string(portal_database_result_value(database_result, 0U, 0U)));
    json_object_object_add(response, "token_type", json_object_new_string("Bearer"));
    json_object_object_add(
        response, "expires_at",
        json_object_new_string(portal_database_result_value(database_result, 0U, 1U)));
    result = portal_http_response_json(connection, MHD_HTTP_OK, response);

cleanup:
    if (response != NULL) {
        json_object_put(response);
        response = NULL;
    }
    portal_database_result_destroy(&database_result);
    return result;
}

enum MHD_Result portal_http_route_logout(PortalHttpContext *context,
                                         struct MHD_Connection *connection) {
    PortalAuthenticatedUser user = {{0}, {0}};
    const char *authorization = NULL;
    const char *values[1] = {NULL};
    PortalDatabaseResult *database_result = NULL;
    enum MHD_Result result = MHD_NO;

    if (portal_http_authenticate(context, connection, &user) != PORTAL_AUTH_AUTHENTICATED) {
        return portal_http_response_error(connection, MHD_HTTP_UNAUTHORIZED, "unauthorized",
                                          "A valid Bearer token is required.");
    }
    authorization =
        MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);
    values[0] = authorization + 7U;
    database_result = portal_database_query(context->database,
                                            "UPDATE auth_sessions SET revoked_at=now() "
                                            "WHERE token=$1::uuid AND revoked_at IS NULL",
                                            1U, values);
    if (portal_database_result_status(database_result) != PORTAL_DATABASE_RESULT_COMMAND) {
        result = auth_database_error(connection, "Could not close the session.");
        goto cleanup;
    }
    result = portal_http_response_empty(connection, MHD_HTTP_NO_CONTENT);

cleanup:
    portal_database_result_destroy(&database_result);
    return result;
}
