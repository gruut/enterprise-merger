cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED COMPONENTS system thread random filesystem unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "test.cpp"
        )

file(GLOB SOURCE_FILES
        "../../src/modules/*/*.cpp"
        "../../src/services/*.cpp"
        "../../src/application.cpp"
        "../../src/chain/*.cpp"
        "../../src/modules/communication/protos/*.cc"
        "../../include/*.cpp"
        )

file(GLOB HEADER_FILES
        "../../src/modules/*/*.hpp"
        "../../src/services/*.hpp"
        "../../src/application.hpp"
        "../../src/module.hpp"
        "../../src/utils/*.hpp"
        "../../src/chain/*.hpp"
        "../../src/config/config.hpp"
        "../../src/modules/communication/protos/*.h"
        "../../include/*.hpp"
        "../../include/*/*.hpp"
        )

set(LIB_PREFIX "/usr/local/lib")
set(LZ4_LIBS "${LIB_PREFIX}/liblz4.a")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(BOTAN_LIBS "${LIB_PREFIX}/libbotan-2.a")
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

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(services_test ${UNIT_TEST_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(services_test PROPERTIES LINKER_LANGUAGE CXX)
target_sources(services_test
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

target_include_directories(services_test PRIVATE ${Boost_INCLUDE_DIR} ../../include ../../lib/leveldb /usr/local/include)

target_link_libraries(services_test
        PRIVATE
        ${Boost_LIBRARIES}
        ${CURL_LIBRARIES}
        leveldb
        ${LZ4_LIBS}
        ${BOTAN_LIBS}
        ${PROTOBUF_LIBS}
        ${GRPC_LIBS}
        )

add_test(
        NAME
        services_test
        COMMAND
        services_test
)
