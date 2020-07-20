project(temperature-server)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(SQLite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -lmicrohttpd10 -ljson-c2")

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

add_executable(
    temperature-server
    temperature-server.c
    ../action/action.c
    ../sqlite/temp.c
    ../map/map_lib.c
    ../util/util.c
    ../util/util.h
)

target_link_libraries(
    temperature-server
    ${SQLITE3_LIBRARIES}
    json-c
)
