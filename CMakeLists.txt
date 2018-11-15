cmake_minimum_required(VERSION 3.12)
project(gruut_hana_merger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-DBOOST_LOG_DYN_LINK)

find_package(Boost REQUIRED COMPONENTS system thread)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(gruut_hana_merger main.cpp)

IF(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
ENDIF(Boost_FOUND)

add_subdirectory(lib/leveldb)
target_include_directories(gruut_hana_merger PRIVATE src/modules include/)
target_link_libraries(gruut_hana_merger
        PRIVATE
            ${Boost_LIBRARIES}
            leveldb
        )

enable_testing()
add_subdirectory(tests/modules)
add_subdirectory(tests/utils)