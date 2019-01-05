cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)

project(gruut_enterprise_merger LANGUAGES CXX)

add_definitions(-DBOOST_LOG_DYN_LINK)

include(cmake/clang-cxx-dev-tools.cmake)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(CURL REQUIRED)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


file(GLOB SOURCE_FILES
        "src/modules/*/*.cpp"
        "src/services/*.cpp"
        "src/application.cpp"
        "src/chain/*.cpp"
        "src/modules/communication/protos/*.cc"
        "include/*.cpp"
        )

file(GLOB HEADER_FILES
        "src/modules/*/*.hpp"
        "src/services/*.hpp"
        "src/application.hpp"
        "src/module.hpp"
        "src/utils/*.hpp"
        "src/chain/*.hpp"
        "src/config/config.hpp"
        "src/modules/communication/protos/*.h"
        "include/*.hpp"
        "include/*/*.hpp"
        )

add_executable(gruut_enterprise_merger main.cpp ${SOURCE_FILES} ${HEADER_FILES})

target_sources(gruut_enterprise_merger
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(LIB_PREFIX "/usr/local/lib")
set(LZ4_LIBS "${LIB_PREFIX}/liblz4.a")
set(BOTAN_LIBS "${LIB_PREFIX}/libbotan-2.a")
set(PROTOBUF_LIBS "${LIB_PREFIX}/protobuf.a")
set(PROTOBUF_LIBS "${LIB_PREFIX}/libprotobuf.a")
if (APPLE)
    set(GRPC_LIBS
            "${LIB_PREFIX}/libgrpc++.dylib"
            "${LIB_PREFIX}/libgrpc.dylib"
            "${LIB_PREFIX}/libgrpc++_cronet.dylib"
            "${LIB_PREFIX}/libgrpc++_error_details.dylib"
            "${LIB_PREFIX}/libgrpc++_reflection.dylib"
            "${LIB_PREFIX}/libgrpc++_unsecure.dylib"
            "${LIB_PREFIX}/libgrpcpp_channelz.dylib")
else ()
    set(GRPC_LIBS
            "${LIB_PREFIX}/libgrpc++.so"
            "${LIB_PREFIX}/libgrpc.so"
            "${LIB_PREFIX}/libgrpc++_cronet.so"
            "${LIB_PREFIX}/libgrpc++_error_details.so"
            "${LIB_PREFIX}/libgrpc++_reflection.so"
            "${LIB_PREFIX}/libgrpc++_unsecure.so"
            "${LIB_PREFIX}/libgrpcpp_channelz.so")
endif ()

IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
ENDIF (Boost_FOUND)

IF (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIR})
ENDIF (CURL_FOUND)

add_subdirectory(lib/leveldb)

target_include_directories(gruut_enterprise_merger PRIVATE include/ /usr/local/include)
target_link_libraries(gruut_enterprise_merger
        PRIVATE
        ${Boost_LIBRARIES}
        ${CURL_LIBRARIES}
        leveldb
        ${LZ4_LIBS}
        ${BOTAN_LIBS}
        ${PROTOBUF_LIBS}
        ${GRPC_LIBS}
        )

IF (${CMAKE_BUILD_TYPE} MATCHES Debug)
    enable_testing()
    add_subdirectory(tests/chain)
    add_subdirectory(tests/modules)
    add_subdirectory(tests/utils)
    add_subdirectory(tests/services)
ENDIF()

IF (NOT $ENV{TRAVIS_BUILD})
    add_custom_command(TARGET gruut_enterprise_merger
            POST_BUILD
            COMMAND make clang-format
            )
ENDIF ()
