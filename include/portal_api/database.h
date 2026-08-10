#ifndef PORTAL_API_DATABASE_H
#define PORTAL_API_DATABASE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct PortalDatabase PortalDatabase;
typedef struct PortalDatabaseResult PortalDatabaseResult;

typedef enum {
    PORTAL_DATABASE_RESULT_ERROR = 0,
    PORTAL_DATABASE_RESULT_COMMAND,
    PORTAL_DATABASE_RESULT_TUPLES
} PortalDatabaseResultStatus;

/** Opens a PostgreSQL connection. Returns NULL on allocation/connect failure. */
PortalDatabase *portal_database_create(const char *connection_string);

/** Closes the connection, frees the handle, and sets *database to NULL. */
void portal_database_destroy(PortalDatabase **database);

bool portal_database_is_connected(const PortalDatabase *database);

/**
 * Executes a parameterized SQL statement.
 *
 * The returned result is owned by the caller even when its status is ERROR.
 * Returns NULL only when a result object cannot be allocated or libpq returns
 * no result. Parameter strings are borrowed for the duration of this call.
 */
PortalDatabaseResult *portal_database_query(PortalDatabase *database, const char *query,
                                            size_t parameter_count, const char *const *parameters);

void portal_database_result_destroy(PortalDatabaseResult **result);
PortalDatabaseResultStatus portal_database_result_status(const PortalDatabaseResult *result);
size_t portal_database_result_row_count(const PortalDatabaseResult *result);

/** Returned value is borrowed and valid until the result is destroyed. */
const char *portal_database_result_value(const PortalDatabaseResult *result, size_t row,
                                         size_t column);

/** Returned message is borrowed and valid until the result is destroyed. */
const char *portal_database_result_error(const PortalDatabaseResult *result);
bool portal_database_result_is_unique_violation(const PortalDatabaseResult *result);

#endif
