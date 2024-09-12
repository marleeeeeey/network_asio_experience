cmake_minimum_required(VERSION 3.15)
project(network_asio_experience VERSION 1.0 LANGUAGES CXX)

# ###################################################################
# #################### Setup compiler options #######################
# ###################################################################
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall -Wextra -Werror -Wpedantic -Wdeprecated -Wextra-semi -Wimplicit-fallthrough -Wconversion
        -Wno-unused-parameter -Wno-unused-lambda-capture -Wno-sign-conversion # TODO remove later
    )
endif()

# ###################################################################
# ############### Searching some packages in system #################
# ###################################################################
find_package(asio CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# ###################################################################
# ################# Add dependencies as submodules ##################
# ###################################################################
add_subdirectory(thirdparty/my_cpp_utils)

# ###################################################################
# ##################### Build projects itself #######################
# ###################################################################
add_subdirectory(first_http_request_asio_example)

add_subdirectory(net_common)
add_subdirectory(simple_common)
add_subdirectory(simple_client)
add_subdirectory(simple_server)
