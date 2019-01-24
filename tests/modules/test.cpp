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

#include "../../src/modules/block_processor/unresolved_block_pool.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_HeaderController)

BOOST_AUTO_TEST_CASE(parseHeader) {
//        string header_added_data("G1123112341234567812345678123456ggg");
//        header_added_data[2] = 0x5A;
//        header_added_data[3] = 0x00;
//        header_added_data[4] = 0x04;
//        header_added_data[6] = 0x00;
//        header_added_data[7] = 0x00;
//        header_added_data[8] = 0x00;
//        header_added_data[9] = 0x23;
//
//        MessageHeader compare_hdr;
//        compare_hdr.identifier = 'G';
//        compare_hdr.version = '1';
//        compare_hdr.message_type = MessageType::MSG_ERROR;
//        compare_hdr.mac_algo_type = MACAlgorithmType::RSA;
//        compare_hdr.compression_algo_type = CompressionAlgorithmType::LZ4;
//        compare_hdr.dummy = '1';
//        for (int i = 0; i < 3; i++)
//                compare_hdr.total_length[i] = 0x00;
//
//        compare_hdr.total_length[3] = 0x23;
//
//        compare_hdr.sender_id.resize(SENDER_ID_LENGTH);
//        for(uint8_t i = '1'; i<='8'; i++) {
//                compare_hdr.local_chain_id[i - '1'] = i;
//                compare_hdr.sender_id[i - '1'] = i;
//        }
//        for(uint8_t i = '1'; i<='6'; i++)
//                compare_hdr.reserved_space[i-'1'] = i;
//
//        MessageHeader origin_hdr = HeaderController::parseHeader(header_added_data);
//
//        BOOST_TEST(origin_hdr.identifier == compare_hdr.identifier);
//        BOOST_TEST(origin_hdr.version == compare_hdr.version);
//        BOOST_TEST(static_cast<uint8_t>(origin_hdr.message_type) == static_cast<uint8_t>(compare_hdr.message_type));
//        BOOST_TEST(static_cast<uint8_t>(origin_hdr.mac_algo_type) == static_cast<uint8_t>(compare_hdr.mac_algo_type));
//        BOOST_TEST(static_cast<uint8_t>(origin_hdr.compression_algo_type) ==
//            static_cast<uint8_t>(compare_hdr.compression_algo_type));
//        BOOST_TEST(origin_hdr.dummy == compare_hdr.dummy);
//        BOOST_TEST(memcmp(origin_hdr.total_length, compare_hdr.total_length, 4) == 0);
//
//        bool is_equal_local_chain_id = origin_hdr.local_chain_id == compare_hdr.local_chain_id;
//        BOOST_TEST(is_equal_local_chain_id);
//
//        bool is_equal_sender_id = origin_hdr.sender_id == compare_hdr.sender_id;
//        BOOST_TEST(is_equal_sender_id);
//        BOOST_TEST(memcmp(origin_hdr.reserved_space, compare_hdr.reserved_space, 6) == 0);
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
        bool true_sample = JsonValidator::validateSchema(sample_true_json, MessageType::MSG_ERROR);
        bool false_sample = JsonValidator::validateSchema(sample_false_json, MessageType::MSG_ERROR);

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

BOOST_AUTO_TEST_SUITE(Test_serialize)
BOOST_AUTO_TEST_CASE(serialize) {

  BasicBlockInfo p_block;
  p_block.merger_id = TypeConverter::integerToBytes(1);
  p_block.chain_id = TypeConverter::integerToArray<CHAIN_ID_TYPE_SIZE>(1);
  p_block.height = 1;
  p_block.transaction_root = vector<uint8_t>(4, 1);

  Transaction test_tx;
  string tx_id_str = "1";
  test_tx.setId(TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(TypeConverter::stringToBytes(tx_id_str)));
  test_tx.setTime(Time::now_int());
  test_tx.setRequestorId(TypeConverter::integerToBytes(1));
  test_tx.setTransactionType(TransactionType::DIGESTS);
  test_tx.setSignature(TypeConverter::integerToBytes(1));
  test_tx.setContents({"Hello world!","Hello world!"});

  p_block.transactions.push_back(test_tx);

  vector<Signature> signature;
  Signature tmp_sig(static_cast<signer_id_type>(TypeConverter::integerToBytes(1)),static_cast<signature_type>(TypeConverter::integerToBytes(1)));
  signature.emplace_back(tmp_sig);

  MerkleTree tree;
  vector<Transaction> transactions = {test_tx};
  tree.generate(transactions);

  Block new_block;
  new_block.initialize(p_block, tree.getMerkleTree());
  new_block.setSupportSignatures(signature);
  new_block.linkPreviousBlock(p_block.prev_id_b64,
                              p_block.prev_hash_b64);
  new_block.finalize();

  json block_header = new_block.getBlockHeaderJson();
  bytes block_raw = new_block.getBlockRaw();
  json block_body = new_block.getBlockBodyJson();

  std::string block_id_b64 = Safe::getString(block_header, "bID");

  // 두번째
  BasicBlockInfo p_block_2;
  p_block_2.merger_id = TypeConverter::integerToBytes(2);
  p_block_2.chain_id = TypeConverter::integerToArray<CHAIN_ID_TYPE_SIZE>(2);
  p_block_2.height = 2;
  p_block_2.transaction_root = vector<uint8_t>(4, 1);

  Transaction test_tx_2;
  string tx_id_str_2 = "2";
  test_tx_2.setId(TypeConverter::bytesToArray<TRANSACTION_ID_TYPE_SIZE>(TypeConverter::stringToBytes(tx_id_str_2)));
  test_tx_2.setTime(Time::now_int());
  test_tx_2.setRequestorId(TypeConverter::integerToBytes(2));
  test_tx_2.setTransactionType(TransactionType::DIGESTS);
  test_tx_2.setSignature(TypeConverter::integerToBytes(2));
  test_tx_2.setContents({"Hello 2!","Hello 2!"});

  p_block_2.transactions.push_back(test_tx_2);

  vector<Signature> signature_2;
  Signature tmp_sig_2(static_cast<signer_id_type>(TypeConverter::integerToBytes(2)),static_cast<signature_type>(TypeConverter::integerToBytes(2)));
  signature_2.emplace_back(tmp_sig_2);

  MerkleTree tree_2;
  vector<Transaction> transactions_2 = {test_tx_2};
  tree_2.generate(transactions_2);

  Block new_block_2;
  new_block_2.initialize(p_block_2, tree_2.getMerkleTree());
  new_block_2.setSupportSignatures(signature_2);
  new_block_2.linkPreviousBlock(p_block_2.prev_id_b64,
                                p_block_2.prev_hash_b64);
  new_block_2.finalize();

  UnresolvedBlockPool test;


  test.serializeUnresolvedBlock(new_block);
  test.serializeUnresolvedBlock(new_block_2);

  test.setUnresolvedBlockCount(2);
  test.saveUnresolvedBlockCount();

  test.saveSerializedUnresolvedBlocks();
  test.restoreUnresolvedBlockPool();

}
BOOST_AUTO_TEST_SUITE_END()