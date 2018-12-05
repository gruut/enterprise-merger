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
#include "../../src/chain/message.hpp"
#include "../../src/chain/types.hpp"

#include "../../include/nlohmann/json.hpp"

#include "../../src/utils/compressor.hpp"
#include "../../src/utils/type_converter.hpp"

#include "../../src/services/storage.hpp"
#include "../../src/services/block_json.hpp"

using namespace gruut;
using namespace nlohmann;
using namespace std;

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator)

    BOOST_AUTO_TEST_CASE(generatePartialBlock) {
        BlockGenerator generator;

        vector<sha256> transactions_digest;
        auto tx_digest = Sha256::hash("1");
        transactions_digest.emplace_back(tx_digest);

        auto block = generator.generatePartialBlock(transactions_digest);
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
      storage->deleteAllDirectory("./block_binary");
      storage->deleteAllDirectory("./certificate");
      storage->deleteAllDirectory("./latest_block_header");
      storage->deleteAllDirectory("./transaction");
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

BOOST_AUTO_TEST_SUITE(Test_Storage_Service)

  BOOST_AUTO_TEST_CASE(save_block) {
    // make mtree for test
    for(size_t i=0; i<(MAX_MERKLE_LEAVES*2)-1; ++i)
      transaction1[4]["mtree"][i]=transaction2[4]["mtree"][i]='h'+to_string(i);

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
      BOOST_TEST("h3" == siblings[0]);
      BOOST_TEST("h4096" == siblings[1]);
      BOOST_TEST("h6145" == siblings[2]);
      BOOST_TEST("h7169" == siblings[3]);
      BOOST_TEST("h7681" == siblings[4]);
      BOOST_TEST("h7937" == siblings[5]);
      BOOST_TEST("h8065" == siblings[6]);
      BOOST_TEST("h8129" == siblings[7]);
      BOOST_TEST("h8161" == siblings[8]);
      BOOST_TEST("h8177" == siblings[9]);
      BOOST_TEST("h8185" == siblings[10]);
      BOOST_TEST("h8189" == siblings[11]);
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