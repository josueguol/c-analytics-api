#include "portal_api/validation.h"

#include <ctype.h>
#include <string.h>

bool portal_validation_key(const char *value, size_t maximum_length) {
    size_t index = 0U;
    size_t length = value == NULL ? 0U : strlen(value);

    if (length == 0U || length > maximum_length) {
        return false;
    }
    for (index = 0U; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];
        if (!(isalnum(character) || character == '-' || character == '_' || character == '.' ||
              character == ':')) {
            return false;
        }
    }
    return true;
}

bool portal_validation_anonymous_id(const char *value) {
    size_t index = 0U;
    size_t length = value == NULL ? 0U : strlen(value);

    if (length < 8U || length > 200U) {
        return false;
    }
    for (index = 0U; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];
        if (!(isalnum(character) || character == '-' || character == '_')) {
            return false;
        }
    }
    return true;
}

bool portal_validation_email(const char *email) {
    const char *at = NULL;
    size_t length = email == NULL ? 0U : strlen(email);

    if (length < 5U || length > 254U || strchr(email, ' ') != NULL) {
        return false;
    }
    at = strchr(email, '@');
    return at != NULL && at != email && at[1] != '\0' && strchr(at + 1, '.') != NULL;
}

bool portal_validation_event_type(const char *event_type) {
    if (event_type == NULL) {
        return false;
    }
    return strcmp(event_type, "page_view") == 0 || strcmp(event_type, "content_view") == 0 ||
           strcmp(event_type, "component_click") == 0 || strcmp(event_type, "like") == 0 ||
           strcmp(event_type, "comment") == 0;
}

bool portal_validation_lowercase_copy(char *target, size_t target_size, const char *source) {
    size_t index = 0U;
    size_t length = source == NULL ? 0U : strlen(source);

    if (target == NULL || source == NULL || target_size <= length) {
        return false;
    }
    for (index = 0U; index < length; ++index) {
        target[index] = (char)tolower((unsigned char)source[index]);
    }
    target[length] = '\0';
    return true;
}
