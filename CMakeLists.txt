cmake_minimum_required(VERSION 3.20)

project(portal_activity_api VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PORTAL_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(PORTAL_ENABLE_CLANG_TIDY "Run clang-tidy while compiling" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED IMPORTED_TARGET libmicrohttpd)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)
pkg_check_modules(JSON_C REQUIRED IMPORTED_TARGET json-c)

set(PORTAL_API_SOURCES
    src/app/app.c
    src/config/config.c
    src/database/database.c
    src/http/authentication.c
    src/http/http.c
    src/http/request.c
    src/http/response.c
    src/http/routes/activity.c
    src/http/routes/analytics.c
    src/http/routes/auth.c
    src/http/routes/favorites.c
    src/validation/validation.c
)

# Flags comunes a todos los targets: sin listas ni bloques duplicados (AGENTS.md 47).
add_library(portal_common_flags INTERFACE)

if(UNIX)
    target_compile_definitions(portal_common_flags INTERFACE _POSIX_C_SOURCE=200809L)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(portal_common_flags INTERFACE -Wall -Wextra -Wpedantic -Werror)
    if(PORTAL_ENABLE_SANITIZERS)
        # Compilación y enlace: instrumentar sólo al enlazar no detecta nada.
        target_compile_options(portal_common_flags INTERFACE
            -fno-omit-frame-pointer -fsanitize=address,undefined
        )
        target_link_options(portal_common_flags INTERFACE -fsanitize=address,undefined)
    endif()
endif()

add_library(portal_api_core STATIC ${PORTAL_API_SOURCES})
target_include_directories(portal_api_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(portal_api_core PRIVATE
    portal_common_flags
    PkgConfig::MICROHTTPD
    PkgConfig::LIBPQ
    PkgConfig::JSON_C
)

add_executable(portal_api src/main.c)
set_target_properties(portal_api PROPERTIES OUTPUT_NAME "portal-api")
target_link_libraries(portal_api PRIVATE portal_common_flags portal_api_core)

if(PORTAL_ENABLE_CLANG_TIDY)
    find_program(PORTAL_CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
    set_target_properties(portal_api_core PROPERTIES
        C_CLANG_TIDY "${PORTAL_CLANG_TIDY_EXECUTABLE}"
    )
endif()

enable_testing()

add_executable(portal_validation_tests tests/test_validation.c)
target_link_libraries(portal_validation_tests PRIVATE portal_common_flags portal_api_core)
add_test(NAME portal_validation_tests COMMAND portal_validation_tests)

find_program(PORTAL_CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(PORTAL_CLANG_FORMAT_EXECUTABLE)
    # Se descubren solas: agregar un archivo no exige actualizar una lista aparte.
    file(GLOB_RECURSE PORTAL_FORMAT_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h
    )
    add_custom_target(format
        COMMAND ${PORTAL_CLANG_FORMAT_EXECUTABLE} -i ${PORTAL_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting C sources"
    )
    add_custom_target(format-check
        COMMAND ${PORTAL_CLANG_FORMAT_EXECUTABLE} --dry-run --Werror
            ${PORTAL_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking C source formatting"
    )
endif()
