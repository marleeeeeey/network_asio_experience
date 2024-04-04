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
find_package(asio CONFIG REQUIRED)

# ###################################################################
# ################# Add dependencies as submodules ##################
# ###################################################################
add_subdirectory(thirdparty/my_cpp_utils)

# ###################################################################
# ##################### Build projects itself #######################
# ###################################################################
add_subdirectory(net_common)
add_subdirectory(simple_client)
add_subdirectory(simple_server)
add_subdirectory(simple_asio_example)
