cmake_minimum_required(VERSION 3.21)
project (galgalim C)

set(SOURCES
    http.c
    main.c
    string.c
    array.c
)

add_executable(galgalim ${SOURCES})
set_property(TARGET galgalim PROPERTY C_STANDARD 11)