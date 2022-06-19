cmake_minimum_required(VERSION 3.21)
project (uriel C)

# add_compile_options(-Wall -Wextra -pedantic -Werror)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


set(SOURCES
    main.c

    cassandra_db.c
    server.c
    router.c
    model.c
    random.c
    string.c
)

add_executable(uriel ${SOURCES})
find_library(CASSANDRA_LIB cassandra)
target_link_libraries(uriel PUBLIC "${CASSANDRA_LIB}")
target_link_libraries(uriel PUBLIC Threads::Threads)
set_property(TARGET uriel PROPERTY C_STANDARD 23)