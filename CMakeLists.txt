cmake_minimum_required(VERSION 3.10)
project(remote_compile C)

set(CMAKE_C_STANDARD 99)

add_custom_target(copy)

add_custom_command(TARGET copy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        copy ${CMAKE_SOURCE_DIR}/Rakefile ${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(rake-c rake-c.c strsplit.c lib.c lib.h)
add_executable(rakeserver rakeserver.c lib.c lib.h)
add_executable(test test.c lib.c lib.h)

