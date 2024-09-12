file(GLOB_RECURSE first_http_request_asio_example_SOURCES "*.cpp")
add_executable(first_http_request_asio_example ${first_http_request_asio_example_SOURCES})

target_link_libraries(first_http_request_asio_example
    PRIVATE
    my_cpp_utils
    asio
)
