#include "http_internal.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

bool portal_http_request_body_append(PortalHttpRequestBody *body, const char *data, size_t size) {
    size_t required = 0U;
    size_t capacity = 0U;
    char *resized = NULL;

    if (body == NULL || data == NULL || size > PORTAL_HTTP_MAX_BODY_SIZE - body->length) {
        return false;
    }
    required = body->length + size + 1U;
    if (required > body->capacity) {
        capacity = body->capacity == 0U ? 4096U : body->capacity;
        while (capacity < required) {
            capacity *= 2U;
        }
        resized = realloc(body->data, capacity);
        if (resized == NULL) {
            return false;
        }
        body->data = resized;
        body->capacity = capacity;
    }
    (void)memcpy(body->data + body->length, data, size);
    body->length += size;
    body->data[body->length] = '\0';
    return true;
}

void portal_http_request_body_destroy(PortalHttpRequestBody **body) {
    if (body == NULL || *body == NULL) {
        return;
    }
    free((*body)->data);
    (*body)->data = NULL;
    free(*body);
    *body = NULL;
}

struct json_object *portal_http_request_parse_json(const PortalHttpRequestBody *body) {
    struct json_tokener *tokener = NULL;
    struct json_object *json = NULL;
    enum json_tokener_error error;
    size_t parse_end = 0U;

    if (body == NULL || body->length == 0U || body->length > (size_t)INT_MAX) {
        return NULL;
    }
    tokener = json_tokener_new();
    if (tokener == NULL) {
        return NULL;
    }
    json = json_tokener_parse_ex(tokener, body->data, (int)body->length);
    error = json_tokener_get_error(tokener);
    parse_end = json_tokener_get_parse_end(tokener);
    while (parse_end < body->length &&
           (body->data[parse_end] == ' ' || body->data[parse_end] == '\t' ||
            body->data[parse_end] == '\r' || body->data[parse_end] == '\n')) {
        ++parse_end;
    }
    json_tokener_free(tokener);
    tokener = NULL;
    if (error != json_tokener_success || parse_end != body->length || json == NULL ||
        !json_object_is_type(json, json_type_object)) {
        if (json != NULL) {
            json_object_put(json);
            json = NULL;
        }
    }
    return json;
}

const char *portal_http_json_string(struct json_object *json, const char *key,
                                    size_t minimum_length, size_t maximum_length) {
    struct json_object *value = NULL;
    const char *text = NULL;
    size_t length = 0U;

    if (json == NULL || key == NULL || !json_object_object_get_ex(json, key, &value) ||
        !json_object_is_type(value, json_type_string)) {
        return NULL;
    }
    text = json_object_get_string(value);
    length = strlen(text);
    return length >= minimum_length && length <= maximum_length ? text : NULL;
}

const char *portal_http_json_optional_string(struct json_object *json, const char *key,
                                             size_t maximum_length) {
    struct json_object *value = NULL;
    const char *text = NULL;

    if (json == NULL || key == NULL || !json_object_object_get_ex(json, key, &value) ||
        !json_object_is_type(value, json_type_string)) {
        return NULL;
    }
    text = json_object_get_string(value);
    return strlen(text) <= maximum_length ? text : NULL;
}
