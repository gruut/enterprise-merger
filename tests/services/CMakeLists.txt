cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS system thread unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "test.cpp"
        )

file(GLOB HEADER_FILES
        "../../src/services/*.hpp"
        "../../src/chain/*.hpp"
        "../../src/utils/*.hpp"
        "../../src/application.hpp"
        )

file(GLOB SOURCE_FILES
        "../../src/services/*.cpp"
        "../../src/application.cpp"
        )

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(services_test ${UNIT_TEST_SOURCE_FILES})
set_target_properties(services_test PROPERTIES LINKER_LANGUAGE CXX)
target_sources(services_test
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

target_include_directories(services_test PRIVATE ${Boost_INCLUDE_DIR} ../../include /usr/local/include)
target_link_libraries(services_test
        PRIVATE
        ${Boost_LIBRARIES}
        /usr/local/lib/libbotan-2.a
        )

add_test(
        NAME
        services_test
        COMMAND
        services_test
)