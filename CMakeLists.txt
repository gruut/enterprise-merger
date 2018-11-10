cmake_minimum_required(VERSION 3.12)
project(gruut_hana_merger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_executable(gruut_hana_merger main.cpp)

# header-only files
add_library(include INTERFACE)
target_include_directories(include INTERFACE include/)
target_link_libraries(gruut_hana_merger
        PRIVATE
            include
        )