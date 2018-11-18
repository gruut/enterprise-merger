cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)

project(gruut_hana_merger LANGUAGES CXX)

add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

find_package(Boost REQUIRED COMPONENTS system thread)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(gruut_hana_merger main.cpp)

file(GLOB SOURCE_FILES
        "src/modules/*/*.cpp"
        "src/services/*.cpp"
        "src/application.cpp"
        )

file(GLOB HEADER_FILES
        "src/modules/*/*.hpp"
        "src/services/*.hpp"
        "src/application.hpp"
        "src/module.hpp"
        )

target_sources(gruut_hana_merger
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
ENDIF (Boost_FOUND)

add_subdirectory(lib/leveldb)
target_include_directories(gruut_hana_merger PRIVATE include/ /usr/local/include)
target_link_libraries(gruut_hana_merger
        PRIVATE
        ${Boost_LIBRARIES}
        leveldb
        /usr/local/lib/libcryptopp.a
        )

enable_testing()
add_subdirectory(tests/modules)
add_subdirectory(tests/utils)
add_subdirectory(tests/services)