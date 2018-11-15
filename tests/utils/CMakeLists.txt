cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS system thread unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "*.cpp"
        )

file(GLOB SRC_FILES
        "../../src/utils/*.hpp"
        )

add_definitions(-DBOOST_TEST_DYN_LINK)

add_executable(utils_test ${UNIT_TEST_SOURCE_FILES} ${SRC_FILES})

set_target_properties(utils_test PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(utils_test PRIVATE ${Boost_INCLUDE_DIR} /usr/local/include)
target_link_libraries(utils_test PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_LIBRARIES} /usr/local/lib/libcryptopp.a)

add_test(
        NAME
        utils_test
        COMMAND
        utils_test
)