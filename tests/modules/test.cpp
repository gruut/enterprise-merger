#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <typeinfo>

#include "../../src/modules/module.hpp"
#include "../../src/application.hpp"
#include "../../src/modules/communication/grpc_util.hpp"
#include "../../src/chain/transaction.hpp"
#include "../../src/modules/storage/storage.hpp"
#include "../../src/modules/storage/block_json.hpp"

using namespace std;
using namespace gruut;
using namespace nlohmann;

BOOST_AUTO_TEST_SUITE(Test_HeaderController)

    BOOST_AUTO_TEST_CASE(attchHeader) {
        MessageHeader msg_hdr;
        msg_hdr.identifier = 'G';
        msg_hdr.version = '1';
        msg_hdr.message_type = MessageType::MSG_ECHO;
        msg_hdr.mac_algo_type = MACAlgorithmType::RSA;
        msg_hdr.compression_algo_type = CompressionAlgorithmType::LZ4;
        msg_hdr.dummy = '1';

        string data("ggg");
        string header_added_data("G1123112341234567812345678123456ggg");
        header_added_data[2] = 0x5A;
        header_added_data[3] = 0x00;
        header_added_data[4] = 0x04;
        header_added_data[6] = 0x00;
        header_added_data[7] = 0x00;
        header_added_data[8] = 0x00;
        header_added_data[9] = 0x23;

        BOOST_TEST(header_added_data ==
                   HeaderController::attachHeader(data, msg_hdr.message_type, msg_hdr.mac_algo_type,
                                                  msg_hdr.compression_algo_type));
    }

    BOOST_AUTO_TEST_CASE(detachHeader) {
        string header_added_data = "11111111111111111111111111111111data";
        string data = "data";
        string header_detached_data = HeaderController::detachHeader(header_added_data);

        BOOST_TEST(header_detached_data == data);
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

    BOOST_AUTO_TEST_CASE(getJsonSize) {
        MessageHeader msg_hdr;
        msg_hdr.total_length[3] = 0x00;
        msg_hdr.total_length[2] = 0x00;
        msg_hdr.total_length[1] = 0x00;
        msg_hdr.total_length[0] = 0x41;

        BOOST_TEST(HeaderController::getJsonSize(msg_hdr) == (65 - HEADER_LENGTH));
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
        BOOST_TEST(memcmp(origin_hdr.local_chain_id, compare_hdr.local_chain_id, 8) == 0);
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

BOOST_AUTO_TEST_SUITE(Test_Storage)

    BOOST_AUTO_TEST_CASE(write_block) {
        Storage storage;
        storage.openDB("./block");

        json block_json = block["block"];
        string block_id = block["block"]["bID"];

        storage.write(block_json, "block", block_id);
        auto data = storage.findBy("block", block_id, "ver").get<string>();

        bool result = data == "4";
        BOOST_TEST(result);
    }

    BOOST_AUTO_TEST_CASE(write_transaction) {
        Storage storage;
        storage.openDB("./transaction");

        json block_body_json = block["tx"];
        string block_id = block["block"]["bID"];

        storage.write(block_body_json, "transactions", block_id);
        auto data = storage.findBy("transactions", "ccccccccc", "type").get<string>();

        bool result = data == "notary";
        BOOST_TEST(result);
    }

    BOOST_AUTO_TEST_CASE(write_latest_block) {
        Storage storage;
        storage.openDB("./latest_block");

        json block_json = block;
        string block_id = block["block"]["bID"];

        storage.write(block_json, "latest_block", block_id);
        auto data = storage.findBy("latest_block", "", "");
        auto result1 = json::parse(data.get<string>());

        auto b_id = result1["block"]["bID"].get<string>();
        BOOST_TEST(b_id == "3fffffffffffffffffff");
    }

    BOOST_AUTO_TEST_CASE(write_cert) {
        Storage storage;
        storage.openDB("./cert");

        json cert_json = cert;
        string n_id = cert["nid"];

        storage.write(cert, "cert", n_id);
        auto data = storage.findBy("cert", "aw98wueiwejnkwe", "").get<string>();

        bool result = data == "sdfnajksdfauweiuaweuiahweiu";
        BOOST_TEST(result);
    }

    BOOST_AUTO_TEST_CASE(write_block_header_hash) {
        Storage storage;
        storage.openDB("./block_header_hash");

        json block_header_hash_json = blkhash;
        string block_id = blkhash["block_id"];

        storage.write(blkhash, "block_header_hash", block_id);
        auto data = storage.findBy("block_header_hash", "123", "").get<string>();

        bool result = data == "oosdmkbjectTobinary";
        BOOST_TEST(result);
    }

    BOOST_AUTO_TEST_CASE(find_txid_pos) {
      string tx_list;
      for (unsigned int idx = 0; idx < block["tx"].size() - 1; ++idx) {
        tx_list += block["tx"][idx]["txID"];
        tx_list += '_';
      }
      block["block"]["txList"] = tx_list;

      Storage storage;
      storage.openDB("./block");

      json block_json = block["block"];
      string block_id = block["block"]["bID"];

      storage.write(block_json, "block", block_id);
      auto data = storage.findTxIdPos("3fffffffffffffffffff", "bbbbbbbbb").get<string>();

      bool result = data == "2";

      BOOST_TEST(result);
    }

    /*  BOOST_AUTO_TEST_CASE(find_sibling){

    }*/

BOOST_AUTO_TEST_SUITE_END()