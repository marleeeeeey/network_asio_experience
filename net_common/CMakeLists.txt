add_library(net_common INTERFACE)

target_include_directories(net_common
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
