cmake_minimum_required(VERSION 3.21)
project (galgalim C)

add_compile_options(-Wall -Wextra -pedantic -Werror)

set(SOURCES
    main.c

    array.c
    file.c
    http.c
    string.c
)

add_executable(galgalim ${SOURCES})
set_property(TARGET galgalim PROPERTY C_STANDARD 23)