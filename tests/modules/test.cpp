#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <typeinfo>

#include "../../src/modules/module.hpp"
#include "../../src/application.hpp"
#include "../../src/modules/communication/grpc_util.hpp"
#include "../../src/modules/communication/http_client.hpp"
#include "../../src/chain/transaction.hpp"
#include "../../src/modules/message_fetcher/message_fetcher.hpp"
#include "../../src/config/config.hpp"

using namespace std;
using namespace gruut;
using namespace nlohmann;

BOOST_AUTO_TEST_SUITE(Test_HeaderController)

    BOOST_AUTO_TEST_CASE(attchHeader) {
        MessageHeader msg_hdr;
        msg_hdr.identifier = 'G';
        msg_hdr.version = '1';
        msg_hdr.message_type = MessageType::MSG_ECHO;
        msg_hdr.mac_algo_type = MACAlgorithmType::NONE;
        msg_hdr.compression_algo_type = config::COMPRESSION_ALGO_TYPE;
        msg_hdr.dummy = '1';

        string data("ggg");
        string header_added_data("G1123112341234567812345678123456ggg");
        header_added_data[2] = 0x5A;
        header_added_data[3] = 0xFF;
        header_added_data[4] = 0x04;
        header_added_data[6] = 0x00;
        header_added_data[7] = 0x00;
        header_added_data[8] = 0x00;
        header_added_data[9] = 0x23;

        auto header_added_data_test = HeaderController::attachHeader(data, msg_hdr.message_type, msg_hdr.compression_algo_type);
        BOOST_TEST(header_added_data == header_added_data_test);
    }

    BOOST_AUTO_TEST_CASE(getMsgBody) {
        string header_added_data = "11111111111111111111111111111111data";
        string data = "data";
        string msg_body = HeaderController::getMsgBody(header_added_data, data.length());

        BOOST_TEST(msg_body == data);
    }

    BOOST_AUTO_TEST_CASE(validateMessage) {
        MessageHeader msg_hdr;
        msg_hdr.identifier = 'G';
        msg_hdr.version = '1';
        msg_hdr.message_type = MessageType::MSG_ECHO;
        msg_hdr.mac_algo_type = MACAlgorithmType::RSA;
        msg_hdr.compression_algo_type = CompressionAlgorithmType::LZ4;
        msg_hdr.dummy = '1';

        BOOST_TEST(HeaderController::validateMessage(msg_hdr));
    }

    BOOST_AUTO_TEST_CASE(getMsgBodySize) {
        MessageHeader msg_hdr;
        msg_hdr.total_length[3] = 0x41;
        msg_hdr.total_length[2] = 0x00;
        msg_hdr.total_length[1] = 0x00;
        msg_hdr.total_length[0] = 0x00;

        BOOST_TEST(HeaderController::getMsgBodySize(msg_hdr) == (65 - HEADER_LENGTH));
    }

    BOOST_AUTO_TEST_CASE(parseHeader) {
        string header_added_data("G1123112341234567812345678123456ggg");
        header_added_data[2] = 0x5A;
        header_added_data[3] = 0x00;
        header_added_data[4] = 0x04;
        header_added_data[6] = 0x00;
        header_added_data[7] = 0x00;
        header_added_data[8] = 0x00;
        header_added_data[9] = 0x23;

        MessageHeader compare_hdr;
        compare_hdr.identifier = 'G';
        compare_hdr.version = '1';
        compare_hdr.message_type = MessageType::MSG_ECHO;
        compare_hdr.mac_algo_type = MACAlgorithmType::RSA;
        compare_hdr.compression_algo_type = CompressionAlgorithmType::LZ4;
        compare_hdr.dummy = '1';
        for (int i = 0; i < 3; i++)
            compare_hdr.total_length[i] = 0x00;
        compare_hdr.total_length[3] = 0x23;
        for (int i = 0; i < 8; i++) {
            compare_hdr.local_chain_id[i] = LOCAL_CHAIN_ID[i];
            compare_hdr.sender_id[i] = SENDER_ID[i];
        }
        for (int i = 0; i < 6; i++)
            compare_hdr.reserved_space[i] = RESERVED[i];

        MessageHeader origin_hdr = HeaderController::parseHeader(header_added_data);

        BOOST_TEST(origin_hdr.identifier == compare_hdr.identifier);
        BOOST_TEST(origin_hdr.version == compare_hdr.version);
        BOOST_TEST(static_cast<uint8_t>(origin_hdr.message_type) == static_cast<uint8_t>(compare_hdr.message_type));
        BOOST_TEST(static_cast<uint8_t>(origin_hdr.mac_algo_type) == static_cast<uint8_t>(compare_hdr.mac_algo_type));
        BOOST_TEST(static_cast<uint8_t>(origin_hdr.compression_algo_type) ==
                   static_cast<uint8_t>(compare_hdr.compression_algo_type));
        BOOST_TEST(origin_hdr.dummy == compare_hdr.dummy);
        BOOST_TEST(memcmp(origin_hdr.total_length, compare_hdr.total_length, 4) == 0);

        bool is_equal_local_chain_id = origin_hdr.local_chain_id == compare_hdr.local_chain_id;
        BOOST_TEST(is_equal_local_chain_id);

        BOOST_TEST(memcmp(origin_hdr.sender_id, compare_hdr.sender_id, 8) == 0);
        BOOST_TEST(memcmp(origin_hdr.reserved_space, compare_hdr.reserved_space, 6) == 0);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_JsonValidator)

    BOOST_AUTO_TEST_CASE(validateSchema) {
        auto sample_true_json = R"(
        {
            "sender":"gruut",
            "time":"2018-11-16"
        }
        )"_json;

        auto sample_false_json = R"(
        {
            "sender":"gruut"
        }
        )"_json;
        bool true_sample = JsonValidator::validateSchema(sample_true_json, MessageType::MSG_ECHO);
        bool false_sample = JsonValidator::validateSchema(sample_false_json, MessageType::MSG_ECHO);

        BOOST_TEST(true_sample);
        BOOST_TEST(!false_sample);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_HttpClient)
  BOOST_AUTO_TEST_CASE(post) {
    HttpClient client("10.10.10.108:3000/api/blocks");

    CURLcode status = client.post("Hello world");
    BOOST_TEST(true);
  }
BOOST_AUTO_TEST_SUITE_END()