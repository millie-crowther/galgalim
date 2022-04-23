cmake_minimum_required(VERSION 3.21)
project (galgalim C)

# add_compile_options(-Wall -Wextra -pedantic -Werror)

set(SOURCES
    main.c

    array.c
    file.c
    http.c
    random.c
    string.c
)

ADD_SUBDIRECTORY(hiredis)
add_executable(galgalim ${SOURCES})
TARGET_LINK_LIBRARIES(galgalim hiredis)
set_property(TARGET galgalim PROPERTY C_STANDARD 23)