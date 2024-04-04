file(GLOB_RECURSE simple_client_SOURCES "*.cpp")
add_executable(simple_client ${simple_client_SOURCES})

target_compile_options(simple_client PRIVATE
    -Wall
    -Wextra

    # -Werror
    # -Wpedantic
)

target_link_libraries(simple_client
    PRIVATE
    my_cpp_utils
    asio
    net_common
)
