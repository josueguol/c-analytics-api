#include "portal_api/app.h"

#include <stdlib.h>

int main(void) {
    PortalApp *app = portal_app_create();
    int result = EXIT_FAILURE;

    if (app != NULL) {
        result = portal_app_run(app) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    portal_app_destroy(&app);
    return result;
}
