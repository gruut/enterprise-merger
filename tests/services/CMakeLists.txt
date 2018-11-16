cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

file(GLOB UNIT_TEST_SOURCE_FILES
        "*.cpp"
        )

file(GLOB SRC_FILES
        "../../src/services/transaction_fetcher.hpp"
        "../../src/chain/transaction.hpp"
        "../../src/chain/*.hpp"
        "../../src/utils/*.hpp"
        )

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(services_test ${UNIT_TEST_SOURCE_FILES} ${SRC_FILES})
set_target_properties(services_test PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(services_test PRIVATE ${Boost_INCLUDE_DIR} ../../include /usr/local/include)
target_link_libraries(services_test
        PRIVATE
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        /usr/local/lib/libcryptopp.a
        )

add_test(
        NAME
        services_test
        COMMAND
        services_test
)