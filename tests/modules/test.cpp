#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <typeinfo>

#include "../../src/modules/module.hpp"
#include "../../src/application.hpp"
#include "../../src/modules/communication/grpc_util.hpp"
#include "../../src/chain/transaction.hpp"
#include "../../src/modules/storage/storage.hpp"
#include "../../src/modules/storage/block_json.hpp"
#include "../../src/modules/message_fetcher/message_fetcher.hpp"

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
        msg_hdr.compression_algo_type = CompressionAlgorithmType::LZ4;
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

BOOST_AUTO_TEST_CASE(save_block) {
  // make mtree for test
  for(size_t i=0; i<(MAX_MERKLE_LEAVES*2)-1; ++i)
    transaction1[4]["mtree"][i]=transaction2[4]["mtree"][i]=to_string(i);

  Storage *storage = Storage::getInstance();
  string block_binary1 = "bbbbbbbbbbbbbinary1";
  storage->saveBlock(block_binary1, block_header1, transaction1);
  string block_binary2 = "bbbbbbbbbbbbbinary2";
  storage->saveBlock(block_binary2, block_header2, transaction2);
  Storage::destroyInstance();

  BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(find_latest_hash_and_height) {
  Storage *storage = Storage::getInstance();
  pair<string, string> hash_and_height = storage->findLatestHashAndHeight();
  Storage::destroyInstance();

  BOOST_TEST(hash_and_height.first == "bbbbbbbbbbbbbinary2");
  BOOST_TEST(hash_and_height.second == "2");
}

BOOST_AUTO_TEST_CASE(find_latest_txid_list) {
  Storage *storage = Storage::getInstance();
  vector<string> tx_ids_list = storage->findLatestTxIdList();
  Storage::destroyInstance();

  BOOST_TEST(tx_ids_list[0] == "QQQQaaaaaaaaa");
  BOOST_TEST(tx_ids_list[1] == "QQQQbbbbbbbbb");
  BOOST_TEST(tx_ids_list[2] == "QQQQccccccccc");
  BOOST_TEST(tx_ids_list[3] == "QQQQddddddddd");
}

BOOST_AUTO_TEST_CASE(find_cert) {
  Storage *storage = Storage::getInstance();
  auto certificate1 = storage->findCertificate("CCC1");
  auto certificate2 = storage->findCertificate("QAAA3");
  auto certificate3 = storage->findCertificate(333); // uint64_t
  Storage::destroyInstance();

  BOOST_TEST(certificate1 == "certC1");
  BOOST_TEST(certificate2 == "certQA3");
  BOOST_TEST(certificate3 == "certA3");
}

BOOST_AUTO_TEST_CASE(read_block_for_block_processor) {
  Storage *storage = Storage::getInstance();

  auto height_metaheader_tx = storage->readBlock(1);
  auto latest_height_metaheader_tx = storage->readBlock(-1);
  Storage::destroyInstance();

  BOOST_TEST(1 == get<0>(height_metaheader_tx));
  BOOST_TEST("bbbbbbbbbbbbbinary1" == get<1>(height_metaheader_tx));
  //BOOST_TEST(transaction1 == get<2>(height_metaheader_tx));

  BOOST_TEST(2 == get<0>(latest_height_metaheader_tx));
  BOOST_TEST("bbbbbbbbbbbbbinary2" == get<1>(latest_height_metaheader_tx));
  //BOOST_TEST(transaction2 == get<2>(latest_height_metaheader_tx));
}

BOOST_AUTO_TEST_CASE(find_sibling) {
  Storage *storage = Storage::getInstance();
  vector<string> siblings = storage->findSibling("QQQQccccccccc"); // tx_pos = 2
  Storage::destroyInstance();
  if(!siblings.empty()){
    BOOST_TEST("3" == siblings[0]);
    BOOST_TEST("4096" == siblings[1]);
    BOOST_TEST("6145" == siblings[2]);
    BOOST_TEST("7169" == siblings[3]);
    BOOST_TEST("7681" == siblings[4]);
    BOOST_TEST("7937" == siblings[5]);
    BOOST_TEST("8065" == siblings[6]);
    BOOST_TEST("8129" == siblings[7]);
    BOOST_TEST("8161" == siblings[8]);
    BOOST_TEST("8177" == siblings[9]);
    BOOST_TEST("8185" == siblings[10]);
    BOOST_TEST("8189" == siblings[11]);
  } else
    BOOST_TEST("EMPTY");
}

BOOST_AUTO_TEST_CASE(delete_all_directory_for_test) {
  Storage *storage = Storage::getInstance();
  storage->deleteAllDirectory("./block_header");
  storage->deleteAllDirectory("./block_binary");
  storage->deleteAllDirectory("./certificate");
  storage->deleteAllDirectory("./latest_block_header");
  storage->deleteAllDirectory("./transaction");
  storage->deleteAllDirectory("./blockid_height");
  Storage::destroyInstance();

  BOOST_TEST(true);
}
BOOST_AUTO_TEST_SUITE_END()