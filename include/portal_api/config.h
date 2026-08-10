#ifndef PORTAL_API_CONFIG_H
#define PORTAL_API_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/** Opaque, immutable application configuration. */
typedef struct PortalConfig PortalConfig;

/**
 * Loads configuration from the environment.
 *
 * DATABASE_URL is required. Invalid optional integer values use documented
 * defaults. The returned object owns copies of all strings.
 */
PortalConfig *portal_config_create_from_environment(void);

/** Frees the configuration and sets *config to NULL. Accepts NULL safely. */
void portal_config_destroy(PortalConfig **config);

/** Returned string is borrowed and valid for the lifetime of config. */
const char *portal_config_database_url(const PortalConfig *config);
uint16_t portal_config_port(const PortalConfig *config);
uint32_t portal_config_token_ttl_hours(const PortalConfig *config);
uint32_t portal_config_confirmation_ttl_hours(const PortalConfig *config);
bool portal_config_exposes_confirmation_token(const PortalConfig *config);

#endif
