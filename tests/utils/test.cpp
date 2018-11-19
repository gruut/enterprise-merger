#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <string>

#include "../../src/utils/sha256.hpp"
#include "../../src/utils/compressor.hpp"

using namespace std;

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

BOOST_AUTO_TEST_SUITE(Test_Compressor)

    BOOST_AUTO_TEST_CASE(compressDataAnddecompressData) {
        string original = "2013-01-07 00:00:04,0.98644,0.98676 2013-01-07 00:01:19,0.98654,0.98676 2013-01-07 00:01:38,0.98644,0.98696";
        int origin_size = original.size();
        string compressed_data, decompressed_data;

        Compressor::compressData(original, compressed_data);
        Compressor::decompressData(compressed_data, decompressed_data, origin_size);

        BOOST_TEST(decompressed_data == original);
    }

BOOST_AUTO_TEST_SUITE_END()