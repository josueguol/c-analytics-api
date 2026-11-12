#include "http_internal.h"

#include <stdio.h>
#include <string.h>

static int test_add_string_rejects_null(void) {
    struct json_object *object = json_object_new_object();
    int failures = 0;

    if (object == NULL) {
        return 1;
    }
    /* Una columna ausente devuelve NULL: agregarla reventaria dentro de
       json_object_new_string(), asi que debe rechazarse antes. */
    if (portal_http_json_add_string(object, "key", NULL)) {
        failures = 1;
    }
    if (json_object_object_length(object) != 0) {
        failures = 1;
    }
    if (!portal_http_json_add_string(object, "key", "value")) {
        failures = 1;
    }
    if (json_object_object_length(object) != 1) {
        failures = 1;
    }
    json_object_put(object);
    return failures;
}

static int test_add_int64_text(void) {
    struct json_object *object = json_object_new_object();
    struct json_object *value = NULL;
    int failures = 0;

    if (object == NULL) {
        return 1;
    }
    if (portal_http_json_add_int64_text(object, "a", NULL) ||
        portal_http_json_add_int64_text(object, "a", "") ||
        portal_http_json_add_int64_text(object, "a", "12abc") ||
        portal_http_json_add_int64_text(object, "a", " 12") ||
        portal_http_json_add_int64_text(object, "a", "9223372036854775808")) {
        failures = 1;
    }
    if (json_object_object_length(object) != 0) {
        failures = 1;
    }
    if (!portal_http_json_add_int64_text(object, "a", "42")) {
        failures = 1;
    }
    if (!json_object_object_get_ex(object, "a", &value) || json_object_get_int64(value) != 42) {
        failures = 1;
    }
    json_object_put(object);
    return failures;
}

static int test_array_append_ownership(void) {
    struct json_object *array = json_object_new_array();
    struct json_object *item = json_object_new_string("item");
    int failures = 0;

    if (array == NULL || item == NULL) {
        json_object_put(array);
        json_object_put(item);
        return 1;
    }
    if (portal_http_json_array_append(array, NULL) || portal_http_json_array_append(NULL, item)) {
        failures = 1;
    }
    /* El rechazo no toma ownership: item sigue vivo y lo agrega el array. */
    if (!portal_http_json_array_append(array, item)) {
        json_object_put(item);
        failures = 1;
    }
    if (json_object_array_length(array) != 1) {
        failures = 1;
    }
    json_object_put(array);
    return failures;
}

static int test_add_object_keeps_ownership_on_failure(void) {
    struct json_object *object = json_object_new_object();
    struct json_object *value = json_object_new_string("value");
    int failures = 0;

    if (object == NULL || value == NULL) {
        json_object_put(object);
        json_object_put(value);
        return 1;
    }
    if (portal_http_json_add_object(NULL, "key", value) ||
        portal_http_json_add_object(object, NULL, value) ||
        portal_http_json_add_object(object, "key", NULL)) {
        failures = 1;
    }
    if (!portal_http_json_add_object(object, "key", value)) {
        json_object_put(value);
        failures = 1;
    }
    json_object_put(object);
    return failures;
}

static int run(const char *name, int (*test)(void)) {
    int failed = test();

    if (failed != 0) {
        (void)fprintf(stderr, "FAILED: %s\n", name);
    }
    return failed;
}

int main(void) {
    int failures = 0;

    failures += run("add_string_rejects_null", test_add_string_rejects_null);
    failures += run("add_int64_text", test_add_int64_text);
    failures += run("array_append_ownership", test_array_append_ownership);
    failures +=
        run("add_object_keeps_ownership_on_failure", test_add_object_keeps_ownership_on_failure);
    if (failures != 0) {
        (void)fprintf(stderr, "%d http json test group(s) failed\n", failures);
        return 1;
    }
    return 0;
}
