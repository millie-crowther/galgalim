cmake_minimum_required(VERSION 3.21)
project (galgalim C)

# add_compile_options(-Wall -Wextra -pedantic -Werror)
# set(INCLUDE_DIR /cpp-driver/include)
# include_directories (${INCLUDE_DIR})

set(SOURCES
    main.c

    array.c
    file.c
    server.c
    router.c
    data_manager.c
    random.c
    string.c
)

ADD_SUBDIRECTORY(hiredis)
# ADD_SUBDIRECTORY(cpp-driver)
add_executable(galgalim ${SOURCES})
find_library(CASSANDRA_LIB cassandra)
message("${CASSANDRA_LIB}")
# TARGET_LINK_LIBRARIES(galgalim hiredis)
TARGET_LINK_LIBRARIES(galgalim PUBLIC hiredis)
target_link_directories(galgalim PUBLIC "/usr/local/lib")
target_link_libraries(galgalim PUBLIC "${CASSANDRA_LIB}")
set_property(TARGET galgalim PROPERTY C_STANDARD 23)