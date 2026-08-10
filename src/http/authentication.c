#include "http_internal.h"

#include <stdio.h>
#include <string.h>

PortalAuthStatus portal_http_authenticate(PortalHttpContext *context,
                                          struct MHD_Connection *connection,
                                          PortalAuthenticatedUser *user) {
    const char *authorization = NULL;
    const char *values[1] = {NULL};
    PortalDatabaseResult *result = NULL;
    PortalAuthStatus status = PORTAL_AUTH_INVALID;
    static const char query[] =
        "SELECT u.id::text, u.email "
        "FROM auth_sessions s JOIN users u ON u.id=s.user_id "
        "WHERE s.token=$1::uuid AND s.revoked_at IS NULL AND s.expires_at > now() "
        "AND u.confirmed_at IS NOT NULL";

    if (context == NULL || connection == NULL || user == NULL) {
        return PORTAL_AUTH_INVALID;
    }
    authorization =
        MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);
    if (authorization == NULL || *authorization == '\0') {
        return PORTAL_AUTH_ANONYMOUS;
    }
    if (strncmp(authorization, "Bearer ", 7U) != 0 ||
        strlen(authorization + 7U) != PORTAL_HTTP_UUID_TEXT_LENGTH) {
        return PORTAL_AUTH_INVALID;
    }
    values[0] = authorization + 7U;
    result = portal_database_query(context->database, query, 1U, values);
    if (portal_database_result_status(result) != PORTAL_DATABASE_RESULT_TUPLES ||
        portal_database_result_row_count(result) != 1U) {
        goto cleanup;
    }
    if (snprintf(user->id, sizeof(user->id), "%s", portal_database_result_value(result, 0U, 0U)) <
            0 ||
        snprintf(user->email, sizeof(user->email), "%s",
                 portal_database_result_value(result, 0U, 1U)) < 0) {
        goto cleanup;
    }
    status = PORTAL_AUTH_AUTHENTICATED;

cleanup:
    portal_database_result_destroy(&result);
    return status;
}
