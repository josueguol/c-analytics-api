#include "portal_api/database.h"

#include <limits.h>
#include <postgresql/libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PortalDatabase {
    PGconn *connection;
};

struct PortalDatabaseResult {
    PGresult *result;
};

PortalDatabase *portal_database_create(const char *connection_string) {
    PortalDatabase *database = NULL;

    if (connection_string == NULL) {
        return NULL;
    }
    database = calloc(1U, sizeof(*database));
    if (database == NULL) {
        return NULL;
    }
    database->connection = PQconnectdb(connection_string);
    if (database->connection == NULL || PQstatus(database->connection) != CONNECTION_OK) {
        if (database->connection != NULL) {
            (void)fprintf(stderr, "Unable to connect to PostgreSQL: %s",
                          PQerrorMessage(database->connection));
        }
        portal_database_destroy(&database);
        return NULL;
    }
    return database;
}

void portal_database_destroy(PortalDatabase **database) {
    if (database == NULL || *database == NULL) {
        return;
    }
    if ((*database)->connection != NULL) {
        PQfinish((*database)->connection);
        (*database)->connection = NULL;
    }
    free(*database);
    *database = NULL;
}

bool portal_database_is_connected(const PortalDatabase *database) {
    return database != NULL && database->connection != NULL &&
           PQstatus(database->connection) == CONNECTION_OK;
}

PortalDatabaseResult *portal_database_query(PortalDatabase *database, const char *query,
                                            size_t parameter_count, const char *const *parameters) {
    PortalDatabaseResult *result = NULL;

    if (database == NULL || database->connection == NULL || query == NULL ||
        parameter_count > (size_t)INT_MAX) {
        return NULL;
    }
    result = calloc(1U, sizeof(*result));
    if (result == NULL) {
        return NULL;
    }
    result->result = PQexecParams(database->connection, query, (int)parameter_count, NULL,
                                  parameters, NULL, NULL, 0);
    if (result->result == NULL) {
        (void)fprintf(stderr, "PostgreSQL query returned no result: %s\n",
                      PQerrorMessage(database->connection));
        portal_database_result_destroy(&result);
        return NULL;
    }
    if (portal_database_result_status(result) == PORTAL_DATABASE_RESULT_ERROR) {
        (void)fprintf(stderr, "PostgreSQL error: %s", portal_database_result_error(result));
    }
    return result;
}

void portal_database_result_destroy(PortalDatabaseResult **result) {
    if (result == NULL || *result == NULL) {
        return;
    }
    if ((*result)->result != NULL) {
        PQclear((*result)->result);
        (*result)->result = NULL;
    }
    free(*result);
    *result = NULL;
}

PortalDatabaseResultStatus portal_database_result_status(const PortalDatabaseResult *result) {
    ExecStatusType status;
    if (result == NULL || result->result == NULL) {
        return PORTAL_DATABASE_RESULT_ERROR;
    }
    status = PQresultStatus(result->result);
    if (status == PGRES_COMMAND_OK) {
        return PORTAL_DATABASE_RESULT_COMMAND;
    }
    if (status == PGRES_TUPLES_OK) {
        return PORTAL_DATABASE_RESULT_TUPLES;
    }
    return PORTAL_DATABASE_RESULT_ERROR;
}

size_t portal_database_result_row_count(const PortalDatabaseResult *result) {
    if (result == NULL || result->result == NULL || PQntuples(result->result) < 0) {
        return 0U;
    }
    return (size_t)PQntuples(result->result);
}

const char *portal_database_result_value(const PortalDatabaseResult *result, size_t row,
                                         size_t column) {
    if (result == NULL || result->result == NULL ||
        row >= portal_database_result_row_count(result) ||
        column >= (size_t)PQnfields(result->result)) {
        return NULL;
    }
    return PQgetvalue(result->result, (int)row, (int)column);
}

const char *portal_database_result_error(const PortalDatabaseResult *result) {
    if (result == NULL || result->result == NULL) {
        return "No PostgreSQL result available.\n";
    }
    return PQresultErrorMessage(result->result);
}

bool portal_database_result_is_unique_violation(const PortalDatabaseResult *result) {
    const char *state = NULL;
    if (result == NULL || result->result == NULL) {
        return false;
    }
    state = PQresultErrorField(result->result, PG_DIAG_SQLSTATE);
    return state != NULL && strcmp(state, "23505") == 0;
}
