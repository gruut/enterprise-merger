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

using namespace gruut;
using namespace nlohmann;
using namespace std;

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator)

    BOOST_AUTO_TEST_CASE(generatePartialBlock) {
//        vector<Transaction> transactions;
//        transactions.push_back(Transaction());
//
//        BlockGenerator generator;
//
//        auto block = generator.generatePartialBlock(sha256());
//
//        bool result = stoi(block.sent_time) > 0;
//        BOOST_TEST(result);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_MessageFactory)

    BOOST_AUTO_TEST_CASE(createSigRequestMessage) {
        PartialBlock block;
        auto output_message = MessageFactory::createSigRequestMessage(block);

        bool result = std::get<0>(output_message) == MessageType::MSG_REQ_SSIG;
        BOOST_TEST(result);

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
