#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <string>
#include "../../src/utils/sha_256.hpp"

BOOST_AUTO_TEST_SUITE(Test_SHA256)
    BOOST_AUTO_TEST_CASE(encrypt) {
        std::string message = "Hello World";
        Sha256::encrypt(message);
        BOOST_TEST(true);
    }

    BOOST_AUTO_TEST_CASE(decrypt) {
        std::string message = "Hello World";
        auto encrypt_message = Sha256::encrypt(message);

        bool result = Sha256::isMatch(message, encrypt_message);
        BOOST_TEST(result);
    }
BOOST_AUTO_TEST_SUITE_END()
