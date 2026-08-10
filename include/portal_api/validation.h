#ifndef PORTAL_API_VALIDATION_H
#define PORTAL_API_VALIDATION_H

#include <stdbool.h>
#include <stddef.h>

bool portal_validation_key(const char *value, size_t maximum_length);
bool portal_validation_anonymous_id(const char *value);
bool portal_validation_email(const char *email);
bool portal_validation_event_type(const char *event_type);

/**
 * Copies source as lowercase ASCII into target.
 * Returns false for NULL arguments or insufficient target capacity.
 */
bool portal_validation_lowercase_copy(char *target, size_t target_size, const char *source);

#endif
