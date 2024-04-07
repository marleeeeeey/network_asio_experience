add_library(simple_common INTERFACE)

target_include_directories(simple_common
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
