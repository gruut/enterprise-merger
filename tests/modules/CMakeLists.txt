cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS system thread unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "test.cpp"
        )

file(GLOB HEADER_FILES
        "../../src/*.hpp"
        "../../src/modules/message_fetcher/*.hpp"
        "../../src/modules/signer_pool_manager/*.hpp"
        "../../src/modules/communication/*.hpp"
        "../../src/modules/communication/protos*.h"
        "../../include/*.hpp"
        )

file(GLOB SOURCE_FILES
        "../../src/modules/communication/*.cpp"
        "../../src/modules/communication/protos/*.cc"
        )

set(LIB_PREFIX "/usr/local/lib")

set(LZ4_LIBS "${LIB_PREFIX}/liblz4.a")

set(PROTOBUF_LIBS "${LIB_PREFIX}/libprotobuf.a")
set(GRPC_LIBS
        "${LIB_PREFIX}/libgrpc++.so"
        "${LIB_PREFIX}/libgrpc.so"
        "${LIB_PREFIX}/libgrpc++_cronet.so"
        "${LIB_PREFIX}/libgrpc++_error_details.so"
        "${LIB_PREFIX}/libgrpc++_reflection.so"
        "${LIB_PREFIX}/libgrpc++_unsecure.so"
        "${LIB_PREFIX}/libgrpcpp_channelz.so"
        )

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(modules_test ${UNIT_TEST_SOURCE_FILES})
set_target_properties(modules_test PROPERTIES LINKER_LANGUAGE CXX)
target_sources(modules_test
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

target_include_directories(modules_test PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(modules_test
        PRIVATE
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_LIBRARIES}
        ${LZ4_LIBS}
        ${PROTOBUF_LIBS}
        ${GRPC_LIBS}
        leveldb
        )

add_test(
        NAME
        modules_test
        COMMAND
        modules_test
)