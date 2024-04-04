cmake_minimum_required(VERSION 3.15)
project(network_asio_experience VERSION 1.0 LANGUAGES CXX)

# ###################################################################
# #################### Setup compiler options #######################
# ###################################################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ###################################################################
# ############### Searching some packages in system #################
# ###################################################################
# find_package(asio CONFIG REQUIRED) // TODO add your packages here

# ###################################################################
# ################# Add dependencies as submodules ##################
# ###################################################################
add_subdirectory(thirdparty/my_cpp_utils)

# ###################################################################
# ##################### Build project itself ########################
# ###################################################################
file(GLOB_RECURSE network_asio_experience_SOURCES "./src/*.cpp")
add_executable(network_asio_experience ${network_asio_experience_SOURCES})

target_compile_options(network_asio_experience PRIVATE
    -Wall
    -Wextra

    # -Werror
    # -Wpedantic
)

target_link_libraries(network_asio_experience
    PRIVATE
    my_cpp_utils
)

target_include_directories(network_asio_experience
    PRIVATE
    src
)
