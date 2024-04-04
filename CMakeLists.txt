cmake_minimum_required(VERSION 3.15)
project(cpp_project_template VERSION 1.0 LANGUAGES CXX)

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
file(GLOB_RECURSE cpp_project_template_SOURCES "./src/*.cpp")
add_executable(cpp_project_template ${cpp_project_template_SOURCES})

target_compile_options(cpp_project_template PRIVATE
    -Wall
    -Wextra

    # -Werror
    # -Wpedantic
)

target_link_libraries(cpp_project_template
    PRIVATE
    my_cpp_utils
)

target_include_directories(cpp_project_template
    PRIVATE
    src
)
