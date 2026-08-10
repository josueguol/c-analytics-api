#include "portal_api/validation.h"

#include <stdio.h>
#include <string.h>

static int test_keys(void) {
    if (!portal_validation_key("science-news:2026", 160U)) {
        return 1;
    }
    if (portal_validation_key("contains/slash", 160U)) {
        return 1;
    }
    if (portal_validation_key("", 160U)) {
        return 1;
    }
    return 0;
}

static int test_anonymous_ids(void) {
    if (!portal_validation_anonymous_id("8de3c6d1-05bb-4b74-82bc-518f1f8b0871")) {
        return 1;
    }
    if (portal_validation_anonymous_id("short")) {
        return 1;
    }
    if (portal_validation_anonymous_id("invalid value")) {
        return 1;
    }
    return 0;
}

static int test_emails(void) {
    if (!portal_validation_email("ana@example.com")) {
        return 1;
    }
    if (portal_validation_email("missing-at.example.com")) {
        return 1;
    }
    if (portal_validation_email("has space@example.com")) {
        return 1;
    }
    return 0;
}

static int test_event_types(void) {
    if (!portal_validation_event_type("component_click")) {
        return 1;
    }
    if (portal_validation_event_type("unknown_event")) {
        return 1;
    }
    return 0;
}

static int test_lowercase_copy(void) {
    char target[32U] = {0};
    char too_small[3U] = {0};

    if (!portal_validation_lowercase_copy(target, sizeof(target), "Ana@EXAMPLE.COM")) {
        return 1;
    }
    if (strcmp(target, "ana@example.com") != 0) {
        return 1;
    }
    if (portal_validation_lowercase_copy(too_small, sizeof(too_small), "long")) {
        return 1;
    }
    return 0;
}

int main(void) {
    if (test_keys() != 0 || test_anonymous_ids() != 0 || test_emails() != 0 ||
        test_event_types() != 0 || test_lowercase_copy() != 0) {
        fprintf(stderr, "validation tests failed\n");
        return 1;
    }
    return 0;
}
