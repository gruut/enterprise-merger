cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)

project(gruut_enterprise_merger LANGUAGES CXX)

add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

find_package(Boost REQUIRED COMPONENTS system thread random)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(gruut_enterprise_merger main.cpp)

file(GLOB SOURCE_FILES
        "src/modules/*/*.cpp"
        "src/services/*.cpp"
        "src/application.cpp"
        "src/chain/*.cpp"
        )

file(GLOB HEADER_FILES
        "src/modules/*/*.hpp"
        "src/services/*.hpp"
        "src/application.hpp"
        "src/module.hpp"
        "src/utils/*.hpp"
        "src/chain/*.hpp"
        )

target_sources(gruut_enterprise_merger
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

set(LIB_PREFIX "/usr/local/lib")
set(LZ4_LIBS "${LIB_PREFIX}/liblz4.a")

IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
ENDIF (Boost_FOUND)

add_subdirectory(lib/leveldb)
target_include_directories(gruut_enterprise_merger PRIVATE include/ /usr/local/include)
target_link_libraries(gruut_enterprise_merger
        PRIVATE
        ${Boost_LIBRARIES}
        leveldb
        ${LZ4_LIBS}
        /usr/local/lib/libbotan-2.a
        )

enable_testing()
add_subdirectory(tests/chain)
add_subdirectory(tests/modules)
add_subdirectory(tests/utils)
add_subdirectory(tests/services)