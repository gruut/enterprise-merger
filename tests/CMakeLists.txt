cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS system thread unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES "modules/message_fetcher/*.cpp")
file(GLOB SRC_FILES
        "../src/*.hpp"
        "../src/modules/message_fetcher/*.hpp"
        )

file(GLOB INCLUDE_FILES
        "../include/*.hpp"
        )

add_definitions(-DBOOST_TEST_DYN_LINK)

add_executable(cpp_test ${UNIT_TEST_SOURCE_FILES} ${SRC_FILES} ${INCLUDE_FILES})

target_include_directories(cpp_test PRIVATE ${Boost_INCLUDE_DIR})

target_link_libraries(cpp_test PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_LIBRARIES})

add_test(
        NAME
        test
        COMMAND
        cpp_test
)