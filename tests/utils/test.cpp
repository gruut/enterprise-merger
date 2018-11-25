#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <string>

#include "../../src/utils/sha256.hpp"
#include "../../src/utils/compressor.hpp"

using namespace std;

BOOST_AUTO_TEST_SUITE(Test_SHA256)

    BOOST_AUTO_TEST_CASE(hash) {
        std::string message = "1";
        auto hashed_list = Sha256::hash(message);

        bool result = Sha256::toString(hashed_list) == "a4ayc/80/OGda4BO/1o/V0etpOqiLx1JwB5S3beHW0s=";
        BOOST_TEST(result);
    }

    BOOST_AUTO_TEST_CASE(isMatch) {
        std::string message = "Hello World";
        auto encrypt_message = Sha256::hash(message);

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