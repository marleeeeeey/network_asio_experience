file(GLOB_RECURSE simple_server_SOURCES "*.cpp")
add_executable(simple_server ${simple_server_SOURCES})

target_compile_options(simple_server PRIVATE
    -Wall
    -Wextra

    # -Werror
    # -Wpedantic
)

target_link_libraries(simple_server
    PRIVATE
    my_cpp_utils
    asio
    net_common
)
