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
        "../../src/modules/storage/storage.hpp"
        "../../include/*.hpp"
        "../../utils/*.hpp"
        "../../src/modules/communication/msg_schema.hpp"
        "../../src/modules/communication/grpc_util.hpp"
        )

file(GLOB SOURCE_FILES
        "../../src/*.cpp"
        "../../src/modules/storage/storage.cpp"
        "../../src/modules/*.cpp"
        "../../src/modules/communication/grpc_util.cpp"
        )

set(LIB_PREFIX "/usr/local/lib")
set(LZ4_LIBS "${LIB_PREFIX}/liblz4.a")
if(APPLE)
    set(GRPC_LIBS "${LIB_PREFIX}/libgrpc++.dylib")
else()
    set(GRPC_LIBS "${LIB_PREFIX}/libgrpc++.so")
endif()

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(modules_test ${UNIT_TEST_SOURCE_FILES})
set_target_properties(modules_test PROPERTIES LINKER_LANGUAGE CXX)
target_sources(modules_test
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

target_include_directories(modules_test PRIVATE ${Boost_INCLUDE_DIR} ../../lib/leveldb ../../include /usr/local/include)
target_link_libraries(modules_test
        PRIVATE
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_LIBRARIES}
        ${LZ4_LIBS}
        leveldb
        /usr/local/lib/libbotan-2.a
        ${GRPC_LIBS}
        )

add_test(
        NAME
        modules_test
        COMMAND
        modules_test
)