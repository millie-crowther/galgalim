cmake_minimum_required(VERSION 3.7)
project (galgalim)

SET(COMPILER_FLAGS "")
SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}")


set(INCLUDE_DIR
)

include_directories(${INCLUDE_DIR})

set(SOURCES
        httpd.c
        main.c
)

add_executable(galgalim ${SOURCES})