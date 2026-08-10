#include "portal_api/config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PortalConfig {
    char *database_url;
    uint16_t port;
    uint32_t token_ttl_hours;
    uint32_t confirmation_ttl_hours;
    bool expose_confirmation_token;
};

static char *config_copy_string(const char *source) {
    char *copy = NULL;
    size_t length = 0;

    if (source == NULL) {
        return NULL;
    }
    length = strlen(source);
    copy = malloc(length + 1U);
    if (copy == NULL) {
        return NULL;
    }
    (void)memcpy(copy, source, length + 1U);
    return copy;
}

static uint32_t config_parse_integer(const char *name, uint32_t fallback, uint32_t minimum,
                                     uint32_t maximum) {
    const char *value = getenv(name);
    char *end = NULL;
    unsigned long parsed = 0;

    if (value == NULL || *value == '\0') {
        return fallback;
    }
    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < minimum || parsed > maximum) {
        (void)fprintf(stderr, "Ignoring invalid %s; using %u\n", name, fallback);
        return fallback;
    }
    return (uint32_t)parsed;
}

static bool config_ascii_equals_ignore_case(const char *left, const char *right) {
    if (left == NULL || right == NULL) {
        return false;
    }
    while (*left != '\0' && *right != '\0') {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == *right;
}

static bool config_parse_boolean(const char *name, bool fallback) {
    const char *value = getenv(name);
    if (value == NULL || *value == '\0') {
        return fallback;
    }
    return strcmp(value, "1") == 0 || config_ascii_equals_ignore_case(value, "true") ||
           config_ascii_equals_ignore_case(value, "yes");
}

PortalConfig *portal_config_create_from_environment(void) {
    const char *database_url = getenv("DATABASE_URL");
    PortalConfig *config = NULL;

    if (database_url == NULL || *database_url == '\0') {
        (void)fprintf(stderr, "DATABASE_URL is required.\n");
        return NULL;
    }
    config = calloc(1U, sizeof(*config));
    if (config == NULL) {
        return NULL;
    }
    config->database_url = config_copy_string(database_url);
    if (config->database_url == NULL) {
        portal_config_destroy(&config);
        return NULL;
    }
    config->port = (uint16_t)config_parse_integer("PORT", 8080U, 1U, 65535U);
    config->token_ttl_hours = config_parse_integer("TOKEN_TTL_HOURS", 168U, 1U, 2160U);
    config->confirmation_ttl_hours = config_parse_integer("CONFIRMATION_TTL_HOURS", 24U, 1U, 720U);
    config->expose_confirmation_token = config_parse_boolean("EXPOSE_CONFIRMATION_TOKEN", false);
    return config;
}

void portal_config_destroy(PortalConfig **config) {
    if (config == NULL || *config == NULL) {
        return;
    }
    free((*config)->database_url);
    (*config)->database_url = NULL;
    free(*config);
    *config = NULL;
}

const char *portal_config_database_url(const PortalConfig *config) {
    return config == NULL ? NULL : config->database_url;
}

uint16_t portal_config_port(const PortalConfig *config) {
    return config == NULL ? 0U : config->port;
}

uint32_t portal_config_token_ttl_hours(const PortalConfig *config) {
    return config == NULL ? 0U : config->token_ttl_hours;
}

uint32_t portal_config_confirmation_ttl_hours(const PortalConfig *config) {
    return config == NULL ? 0U : config->confirmation_ttl_hours;
}

bool portal_config_exposes_confirmation_token(const PortalConfig *config) {
    return config != NULL && config->expose_confirmation_token;
}
