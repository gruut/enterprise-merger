cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED COMPONENTS system thread random filesystem unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "test.cpp"
        )

file(GLOB HEADER_FILES
        "../../include/nlohmann/json.hpp"
        "../../src/services/*.hpp"
        "../../src/chain/*.hpp"
        "../../src/utils/*.hpp"
        "../../src/modules/storage/storage.hpp"
        "../../src/application.hpp"
        "../../src/utils/template_singleton.hpp"
        )

file(GLOB SOURCE_FILES
        "../../src/chain/*.cpp"
        "../../src/services/*.cpp"
        "../../src/application.cpp"
        "../../src/modules/storage/storage.cpp"
        )

set(LIB_PREFIX "/usr/local/lib")
set(LZ4_LIBS "${LIB_PREFIX}/liblz4.a")

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(services_test ${UNIT_TEST_SOURCE_FILES})
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
        ${LZ4_LIBS}
        /usr/local/lib/libbotan-2.a
        leveldb
        )

add_test(
        NAME
        services_test
        COMMAND
        services_test
)