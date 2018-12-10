#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <vector>

#include "../../src/application.hpp"

#include "../../src/services/signer_pool_manager.hpp"
#include "../../src/services/block_generator.hpp"
#include "../../src/services/message_factory.hpp"
#include "../../src/services/input_queue.hpp"
#include "../../src/services/output_queue.hpp"
#include "../../src/services/transaction_pool.hpp"

#include "../../src/chain/transaction.hpp"
#include "../../src/chain/signature.hpp"
#include "../../src/chain/message.hpp"
#include "../../src/chain/types.hpp"

#include "../../include/nlohmann/json.hpp"

#include "../../src/utils/compressor.hpp"
#include "../../src/utils/type_converter.hpp"
#include "../../src/utils/time.hpp"

#include "../../src/services/storage.hpp"
#include "../../src/services/block_json.hpp"

using namespace gruut;
using namespace nlohmann;
using namespace std;
using namespace macaron;

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator)

    BOOST_AUTO_TEST_CASE(generatePartialBlock) {
        BlockGenerator generator;

        vector<sha256> transactions_digest;
        auto tx_digest = Sha256::hash("1");
        transactions_digest.emplace_back(tx_digest);

        vector<Transaction> transactions;
        transactions.emplace_back(Transaction());

        auto block = generator.generatePartialBlock(transactions_digest, transactions);
        BOOST_TEST(true);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_MessageFactory)

    BOOST_AUTO_TEST_CASE(createSigRequestMessage) {
//        PartialBlock block;
//        auto output_message = MessageFactory::createSigRequestMessage(block);
//
//        bool result = std::get<0>(output_message) == MessageType::MSG_REQ_SSIG;
//        BOOST_TEST(result);

        // TODO: src/services/message_factory.hpp:30 해결하면 주석해제할 것
//        json j_string2;
//        j_string2["cID"] = "";
//        j_string2["hgt"] = "";
//        j_string2["mID"] = "";
//        j_string2["time"] = "";
//        j_string2["txrt"] = "";
//
//        result = message.data == j_string2;
//        BOOST_TEST(result);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_SignerPool)

    BOOST_AUTO_TEST_CASE(pushSigner) {
      SignerPool signer_pool;

      signer_id_type id = 1;
      string test_cert = "MIIC7zCCAdegAwIBAgIBADANBgkqhkiG9w0BAQsFADBhMRQwEgYDVQQDEwt0aGVWYXVsdGVyczELMAkGA1UEBhMCS1IxCTAHBgNVBAgTADEQMA4GA1UEBxMHSW5jaGVvbjEUMBIGA1UEChMLdGhlVmF1bHRlcnMxCTAHBgNVBAsTADAeFw0xODExMjgwNTM1NDFaFw0xOTExMjgwNTM1NDFaMBUxEzARBgNVBAMMCkdSVVVUX0FVVEgwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDV2RKC+oo6sBeAoSJn55ZZJ+U9bRh4z/TOsc4V/92NsV5qXpiWhUMTqPfNGHTjR7ScI57ZH9lqltcBJ2mcqhBWY1A/lQfdWAJf+3/eh+H/ZvDcZW8s9PFeuJcftmEDtUMlh9xMUoL5a74dS5lhrdbH0tXRMfhB3w02fmkuvqW+MCsUubhL7mu0PDbJeWjqqu8P+c+6PWO0CRgkMmry1f1VksXTzp54wARW2O3Zut6Z56VknrMOP2f4IYGiLy8zC/oO/JRCPCFvW1cM5UDdjVaq8UkIZ7B/z4zqFjwT3gXHHdMp+RLS8t+tA15rhZ2iRtKPcSwlpV95BTBG3Jpbm7xTAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAEAzwa2yTQMR6nRUgafc/v0z7PylG4ohkXljBzMxfFipYju1/AKse1PCBq2H9DSnSbeL/BI4lQjsXXJLCDSnWXNnJSt1oatOHv4JeJ2Ob88EBVkx7G0aK2S2yijfMx5Bpptp8FIYxZX0QuOJ2oNK73j1Dx9Xax+5ZkBE8wxYYXpsZ0R/BGw8Es1bNFyFcbNYWd3iQOwoXOenWWa6YOyzRhZ2EAw+l7C7LB6I68xIIAP0BBSMTOfq4Smdizdd3qWYJyouUcv83AZn8KWBJjRKNJgHQvnYzCCGnhOwekbh9WlrGVEUvr/b6yV/aXX6kMqsCAfLhloqQ7Ai24QvOfdOAEQ=";
      vector<uint8_t> secret_key(32, 0);
      auto secret_key_vector = TypeConverter::toSecureVector(secret_key);
      signer_pool.pushSigner(4, test_cert, secret_key_vector, SignerStatus::GOOD);

      BOOST_CHECK_EQUAL(signer_pool.size(), 1);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_MessageIOQueues)

  BOOST_AUTO_TEST_CASE(pushMessages) {
    nlohmann::json msg_body = "{}"_json;
    std::vector<std::string> msg_receiver = {};

    InputQueueAlt * input_queue = InputQueueAlt::getInstance();
    InputMsgEntry test_input_msg(MessageType::MSG_ACCEPT, msg_body);
    input_queue->push(test_input_msg);

    OutputQueueAlt * output_queue = OutputQueueAlt::getInstance();
    OutputMsgEntry test_output_msg(MessageType::MSG_ACCEPT, msg_body, msg_receiver);
    output_queue->push(test_output_msg);

    bool test_result = (!input_queue->empty() && !output_queue->empty());

    InputQueueAlt::destroyInstance();
    OutputQueueAlt::destroyInstance();

    BOOST_TEST(test_result);
  }

    BOOST_AUTO_TEST_CASE(create_transactions) {
      SignerPool signer_pool;

      string test_cert = "MIIC7zCCAdegAwIBAgIBADANBgkqhkiG9w0BAQsFADBhMRQwEgYDVQQDEwt0aGVWYXVsdGVyczELMAkGA1UEBhMCS1IxCTAHBgNVBAgTADEQMA4GA1UEBxMHSW5jaGVvbjEUMBIGA1UEChMLdGhlVmF1bHRlcnMxCTAHBgNVBAsTADAeFw0xODExMjgwNTM1NDFaFw0xOTExMjgwNTM1NDFaMBUxEzARBgNVBAMMCkdSVVVUX0FVVEgwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDV2RKC+oo6sBeAoSJn55ZZJ+U9bRh4z/TOsc4V/92NsV5qXpiWhUMTqPfNGHTjR7ScI57ZH9lqltcBJ2mcqhBWY1A/lQfdWAJf+3/eh+H/ZvDcZW8s9PFeuJcftmEDtUMlh9xMUoL5a74dS5lhrdbH0tXRMfhB3w02fmkuvqW+MCsUubhL7mu0PDbJeWjqqu8P+c+6PWO0CRgkMmry1f1VksXTzp54wARW2O3Zut6Z56VknrMOP2f4IYGiLy8zC/oO/JRCPCFvW1cM5UDdjVaq8UkIZ7B/z4zqFjwT3gXHHdMp+RLS8t+tA15rhZ2iRtKPcSwlpV95BTBG3Jpbm7xTAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAEAzwa2yTQMR6nRUgafc/v0z7PylG4ohkXljBzMxfFipYju1/AKse1PCBq2H9DSnSbeL/BI4lQjsXXJLCDSnWXNnJSt1oatOHv4JeJ2Ob88EBVkx7G0aK2S2yijfMx5Bpptp8FIYxZX0QuOJ2oNK73j1Dx9Xax+5ZkBE8wxYYXpsZ0R/BGw8Es1bNFyFcbNYWd3iQOwoXOenWWa6YOyzRhZ2EAw+l7C7LB6I68xIIAP0BBSMTOfq4Smdizdd3qWYJyouUcv83AZn8KWBJjRKNJgHQvnYzCCGnhOwekbh9WlrGVEUvr/b6yV/aXX6kMqsCAfLhloqQ7Ai24QvOfdOAEQ=";
      vector<uint8_t> secret_key(32, 0);
      auto secret_key_vector = TypeConverter::toSecureVector(secret_key);
      signer_pool.pushSigner(1, test_cert, secret_key_vector, SignerStatus::GOOD);

      signer_pool.createTransactions();
      auto transaction_pool_size = Application::app().getTransactionPool().size();
      BOOST_CHECK_EQUAL(transaction_pool_size, 1);
    }

    BOOST_AUTO_TEST_CASE(delete_all_directory_for_test) {
      Storage *storage = Storage::getInstance();
      storage->deleteAllDirectory("./block_header");
      storage->deleteAllDirectory("./block_meta_header");
      storage->deleteAllDirectory("./certificate");
      storage->deleteAllDirectory("./latest_block_header");
      storage->deleteAllDirectory("./block_body");
      storage->deleteAllDirectory("./blockid_height");
      Storage::destroyInstance();

      BOOST_TEST(true);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_TransactionPool)
    BOOST_AUTO_TEST_CASE(push) {
      TransactionPool transaction_pool;
      Transaction transaction;
      transaction_pool.push(transaction);

      BOOST_CHECK_EQUAL(transaction_pool.size(), 1);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_SignaturePool)
  BOOST_AUTO_TEST_CASE(fetchN) {
    SignaturePool signature_pool;
    Signature signature1;
    Signature signature2;

    signature_pool.push(signature1);
    signature_pool.push(signature2);

    auto signatures = signature_pool.fetchN(2);

    BOOST_CHECK_EQUAL(signatures.size(), 2);
  }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_Storage_Service)

  BOOST_AUTO_TEST_CASE(save_block) {
    // make mtree for test
    for(size_t i=0; i<(MAX_MERKLE_LEAVES*2)-1; ++i)
      block_body1["mtree"][i]=block_body2["mtree"][i]='h'+to_string(i);

    Storage *storage = Storage::getInstance();
    string block_meta_header1 = Base64().Encode(block_header1.dump()); // for test
    storage->saveBlock(block_meta_header1, block_header1, block_body1);
    string block_meta_header2 = Base64().Encode(block_header2.dump()); // for test
    storage->saveBlock(block_meta_header2, block_header2, block_body2);
    Storage::destroyInstance();

    BOOST_TEST(true);
  }

  BOOST_AUTO_TEST_CASE(save_block_by_block_object) {
    Storage *storage = Storage::getInstance();

    PartialBlock p_block;
    p_block.merger_id = 1;
    p_block.chain_id = 1;
    p_block.height = "1";
    p_block.transaction_root = vector<uint8_t>(4, 1);

    Transaction t;
    string tx_id_str = "1";
    t.transaction_id = TypeConverter::stringToBytes(tx_id_str);
    auto now = Time::now();
    t.sent_time = TypeConverter::stringToBytes(now);
    t.requestor_id = TypeConverter::toBytes(1);
    t.transaction_type = TransactionType::DIGESTS;
    t.signature = TypeConverter::toBytes(1);
    t.content_list.emplace_back("Hello world!");

    p_block.transactions.push_back(t);

    Block block(p_block);
    block.compression_algo_type = CompressionAlgorithmType::LZ4;
    block.header_length = 1;

    block.version = 1;
    block.previous_header_hash = Sha256::hash("1");
    block.previous_block_id = Sha256::hash("1");
    block.block_id = Sha256::hash("1");
    block.timestamp = Time::now_int();

    Signature signature;
    signature.signer_id = 1;
    signature.signer_signature = TypeConverter::toBytes(1);

    block.signer_signatures.push_back(signature);
    block.signature = TypeConverter::toBytes(1);

    MerkleTree tree;
    vector<sha256> tx_digest = {t.transaction_id};
    tree.generate(tx_digest);
    block.merkle_tree = tree;
    block.transactions_count = block.transactions.size();

    storage->saveBlock(block);

    auto hash_and_height = storage->findLatestHashAndHeight();
    BOOST_CHECK_EQUAL(hash_and_height.second, "1");

    auto latest_list = storage->findLatestTxIdList();
    auto tx_id = latest_list[0];

    string encoded_tx_id = TypeConverter::toBase64Str(t.transaction_id);
    BOOST_CHECK_EQUAL(tx_id, encoded_tx_id);

    Storage::destroyInstance();
  }

//  BOOST_AUTO_TEST_CASE(find_latest_hash_and_height) {
//    Storage *storage = Storage::getInstance();
//    pair<string, string> hash_and_height = storage->findLatestHashAndHeight();
//    Storage::destroyInstance();
//
//    // for test
//    Sha256 sha;
//    block_header1.erase("txids");
//    block_header2.erase("txids");
//    sha256 hash = sha.hash(Base64().Encode(block_header2.dump()));
//    BOOST_TEST(hash_and_height.first == sha.toString(hash));
//    BOOST_TEST(hash_and_height.second == "2");
//  }

//  BOOST_AUTO_TEST_CASE(find_latest_txid_list) {
//    Storage *storage = Storage::getInstance();
//    vector<string> tx_ids_list = storage->findLatestTxIdList();
//    Storage::destroyInstance();
//
//    BOOST_TEST(tx_ids_list[0] == "Qa");
//    BOOST_TEST(tx_ids_list[1] == "Qb");
//    BOOST_TEST(tx_ids_list[2] == "Qc");
//    BOOST_TEST(tx_ids_list[3] == "Qd");
//  }

//  BOOST_AUTO_TEST_CASE(find_cert) {
//    Storage *storage = Storage::getInstance();
//    auto certificate1 = storage->findCertificate("Qc4");
//    auto certificate2 = storage->findCertificate("c1");
//    //auto certificate3 = storage->findCertificate(333); // uint64_t
//    Storage::destroyInstance();
//
//    BOOST_TEST(certificate1 == "certQc4");
//    BOOST_TEST(certificate2 == "certc1");
//    //BOOST_TEST(certificate3 == "certA3");
//  }
//
//  BOOST_AUTO_TEST_CASE(read_block_for_block_processor) {
//    Storage *storage = Storage::getInstance();
//
//    auto height_metaheader_tx = storage->readBlock(1);
//    auto latest_height_metaheader_tx = storage->readBlock(-1);
//    auto no_data_metaheader_tx = storage->readBlock(9999);
//    Storage::destroyInstance();
//
//    BOOST_TEST(1 == get<0>(height_metaheader_tx));
//    BOOST_TEST(Base64().Encode(block_header1.dump()) == get<1>(height_metaheader_tx));
//    //cout << get<2>(height_metaheader_tx) << endl;
//
//    BOOST_TEST(2 == get<0>(latest_height_metaheader_tx));
//    BOOST_TEST(Base64().Encode(block_header2.dump()) == get<1>(latest_height_metaheader_tx));
//    //cout << get<2>(latest_height_metaheader_tx) <<endl;
//
//    BOOST_TEST(-1 == get<0>(no_data_metaheader_tx));
//    BOOST_TEST("" == get<1>(no_data_metaheader_tx));
//    BOOST_TEST("" == get<2>(no_data_metaheader_tx));
//  }
//
//  BOOST_AUTO_TEST_CASE(find_sibling) {
//    Storage *storage = Storage::getInstance();
//    vector<string> siblings = storage->findSibling("c"); // tx_pos = 2
//    Storage::destroyInstance();
//    if(!siblings.empty()){
//      BOOST_TEST("h3" == siblings[0]);
//      BOOST_TEST("h4096" == siblings[1]);
//      BOOST_TEST("h6145" == siblings[2]);
//      BOOST_TEST("h7169" == siblings[3]);
//      BOOST_TEST("h7681" == siblings[4]);
//      BOOST_TEST("h7937" == siblings[5]);
//      BOOST_TEST("h8065" == siblings[6]);
//      BOOST_TEST("h8129" == siblings[7]);
//      BOOST_TEST("h8161" == siblings[8]);
//      BOOST_TEST("h8177" == siblings[9]);
//      BOOST_TEST("h8185" == siblings[10]);
//      BOOST_TEST("h8189" == siblings[11]);
//    } else
//      BOOST_TEST("EMPTY");
//  }
//
//  BOOST_AUTO_TEST_CASE(delete_all_directory_for_test) {
//    Storage *storage = Storage::getInstance();
//    storage->deleteAllDirectory("./block_header");
//    storage->deleteAllDirectory("./block_meta_header");
//    storage->deleteAllDirectory("./certificate");
//    storage->deleteAllDirectory("./latest_block_header");
//    storage->deleteAllDirectory("./block_body");
//    storage->deleteAllDirectory("./blockid_height");
//    Storage::destroyInstance();
//
//    BOOST_TEST(true);
//  }
BOOST_AUTO_TEST_SUITE_END()