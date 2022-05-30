cmake_minimum_required(VERSION 3.21)
project (galgalim C)

# add_compile_options(-Wall -Wextra -pedantic -Werror)

set(SOURCES
    main.c

    array.c
    database.c
    file.c
    server.c
    router.c
    model.c
    random.c
    string.c
)

ADD_SUBDIRECTORY(hiredis)
add_executable(galgalim ${SOURCES})
find_library(CASSANDRA_LIB cassandra)
target_link_libraries(galgalim PUBLIC hiredis)
target_link_libraries(galgalim PUBLIC "${CASSANDRA_LIB}")
set_property(TARGET galgalim PROPERTY C_STANDARD 23)