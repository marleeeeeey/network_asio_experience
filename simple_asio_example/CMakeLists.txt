file(GLOB_RECURSE simple_asio_example_SOURCES "*.cpp")
add_executable(simple_asio_example ${simple_asio_example_SOURCES})

target_compile_options(simple_asio_example PRIVATE
    -Wall
    -Wextra

    # -Werror
    # -Wpedantic
)

target_link_libraries(simple_asio_example
    PRIVATE
    my_cpp_utils
    asio
)
