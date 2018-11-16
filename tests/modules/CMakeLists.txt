cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS system thread unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "test.cpp"
        )

file(GLOB SRC_FILES
        "../../src/*.hpp"
        "../../src/modules/message_fetcher/*.hpp"
        "../../src/modules/signer_pool_manager/*.hpp"
        "../../include/nlohmann/json.hpp"
        )

set(PREFIX "/usr/local")
set(LZ4_LIB_DIR "${PREFIX}/lib")
set(LZ4_LIBS "${LZ4_LIB_DIR}/liblz4.a")

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(modules_test ${UNIT_TEST_SOURCE_FILES} ${SRC_FILES})
set_target_properties(modules_test PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(modules_test PRIVATE ${Boost_INCLUDE_DIR} /usr/local/include)
target_link_libraries(modules_test
        PRIVATE
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_LIBRARIES}
        ${LZ4_LIBS}
        leveldb
        /usr/local/lib/libcryptopp.a
        )

add_test(
        NAME
        modules_test
        COMMAND
        modules_test
)