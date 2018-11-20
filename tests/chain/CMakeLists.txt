cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "test.cpp"
        )

file(GLOB HEADER_FILES
        "../../src/chain/*.hpp"
        )

file(GLOB SOURCE_FILES
        "../../src/chain/*.cpp"
        )

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(chain_test ${UNIT_TEST_SOURCE_FILES})
set_target_properties(chain_test PROPERTIES LINKER_LANGUAGE CXX)
target_sources(chain_test
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )

target_include_directories(chain_test PRIVATE ${Boost_INCLUDE_DIR} /usr/local/include)
target_link_libraries(chain_test
        PRIVATE
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        /usr/local/lib/libbotan-2.a
        )

add_test(
        NAME
        chain_test
        COMMAND
        chain_test
)