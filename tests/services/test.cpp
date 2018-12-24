#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <vector>

#include "../../src/chain/transaction.hpp"
#include "../../src/chain/signature.hpp"
#include "../../src/chain/message.hpp"
#include "../../src/chain/types.hpp"

#include "../../include/nlohmann/json.hpp"

#include "../../src/services/block_generator.hpp"
#include "../../src/services/input_queue.hpp"
#include "../../src/services/output_queue.hpp"

#include "../../src/utils/compressor.hpp"
#include "../../src/utils/type_converter.hpp"

#include "../../src/services/storage.hpp"
#include "block_json.hpp"

#include "fixture.hpp"

using namespace gruut;
using namespace nlohmann;
using namespace std;

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator)

    BOOST_AUTO_TEST_CASE(generatePartialBlock) {
        BlockGenerator generator;

        auto tx_digest = Sha256::hash("1");

        vector<Transaction> transactions;
        transactions.emplace_back(Transaction());

        auto block = generator.generatePartialBlock(tx_digest, transactions);
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

BOOST_FIXTURE_TEST_SUITE(Test_SignerPool, SignerPoolFixture)

    BOOST_AUTO_TEST_CASE(pushSigner) {
      SignerPoolFixture signer_pool_fixture;
      signer_pool_fixture.push();
      BOOST_CHECK_EQUAL(signer_pool_fixture.signer_pool.size(), 1);
    }

    BOOST_AUTO_TEST_CASE(updateStatus) {
      SignerPoolFixture signer_pool_fixture;
      signer_pool_fixture.push();
      signer_id_type prev_signer_id = TypeConverter::integerToBytes(signer_pool_fixture.id_int - 1);
      signer_pool_fixture.signer_pool.updateStatus(prev_signer_id, SignerStatus::TEMPORARY);

      auto signer = signer_pool_fixture.signer_pool.getSigner(0);
      bool result = signer.status == SignerStatus::TEMPORARY;
      BOOST_TEST(result);
    }

  BOOST_AUTO_TEST_CASE(getNumSignerBy) {
    SignerPoolFixture signer_pool_fixture;
    signer_pool_fixture.push();
    signer_pool_fixture.push();
    signer_pool_fixture.push();
    signer_pool_fixture.push();

    auto signers = signer_pool_fixture.signer_pool.getRandomSigners(2);
    BOOST_CHECK_EQUAL(signers.size(), 2);
  }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_MessageIOQueues)

  BOOST_AUTO_TEST_CASE(pushMessages) {
    nlohmann::json msg_body = "{}"_json;
    std::vector<id_type> msg_receiver = {};

    auto input_queue = InputQueueAlt::getInstance();
    InputMsgEntry test_input_msg(MessageType::MSG_ACCEPT, msg_body);
    input_queue->push(test_input_msg);

    auto output_queue = OutputQueueAlt::getInstance();
    OutputMsgEntry test_output_msg(MessageType::MSG_ACCEPT, msg_body, msg_receiver);
    output_queue->push(test_output_msg);

    bool test_result = (!input_queue->empty() && !output_queue->empty());

    BOOST_TEST(test_result);
  }

//  BOOST_AUTO_TEST_CASE(create_transactions) {
//    SignerPool signer_pool;
//
//    string test_cert = "MIIC7zCCAdegAwIBAgIBADANBgkqhkiG9w0BAQsFADBhMRQwEgYDVQQDEwt0aGVWYXVsdGVyczELMAkGA1UEBhMCS1IxCTAHBgNVBAgTADEQMA4GA1UEBxMHSW5jaGVvbjEUMBIGA1UEChMLdGhlVmF1bHRlcnMxCTAHBgNVBAsTADAeFw0xODExMjgwNTM1NDFaFw0xOTExMjgwNTM1NDFaMBUxEzARBgNVBAMMCkdSVVVUX0FVVEgwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDV2RKC+oo6sBeAoSJn55ZZJ+U9bRh4z/TOsc4V/92NsV5qXpiWhUMTqPfNGHTjR7ScI57ZH9lqltcBJ2mcqhBWY1A/lQfdWAJf+3/eh+H/ZvDcZW8s9PFeuJcftmEDtUMlh9xMUoL5a74dS5lhrdbH0tXRMfhB3w02fmkuvqW+MCsUubhL7mu0PDbJeWjqqu8P+c+6PWO0CRgkMmry1f1VksXTzp54wARW2O3Zut6Z56VknrMOP2f4IYGiLy8zC/oO/JRCPCFvW1cM5UDdjVaq8UkIZ7B/z4zqFjwT3gXHHdMp+RLS8t+tA15rhZ2iRtKPcSwlpV95BTBG3Jpbm7xTAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAEAzwa2yTQMR6nRUgafc/v0z7PylG4ohkXljBzMxfFipYju1/AKse1PCBq2H9DSnSbeL/BI4lQjsXXJLCDSnWXNnJSt1oatOHv4JeJ2Ob88EBVkx7G0aK2S2yijfMx5Bpptp8FIYxZX0QuOJ2oNK73j1Dx9Xax+5ZkBE8wxYYXpsZ0R/BGw8Es1bNFyFcbNYWd3iQOwoXOenWWa6YOyzRhZ2EAw+l7C7LB6I68xIIAP0BBSMTOfq4Smdizdd3qWYJyouUcv83AZn8KWBJjRKNJgHQvnYzCCGnhOwekbh9WlrGVEUvr/b6yV/aXX6kMqsCAfLhloqQ7Ai24QvOfdOAEQ=";
//    vector<uint8_t> secret_key(32, 0);
//    auto secret_key_vector = TypeConverter::toSecureVector(secret_key);
//    signer_id_type signer_id = TypeConverter::integerToBytes(1);
//    signer_pool.pushSigner(signer_id, test_cert, secret_key_vector, SignerStatus::GOOD);
//
//    signer_pool.createTransactions();
//    auto transaction_pool_size = Application::app().getTransactionPool().size();
//    BOOST_CHECK_EQUAL(transaction_pool_size, 1);
//    BOOST_CHECK_EQUAL(1, 1);
//  }

  BOOST_AUTO_TEST_CASE(delete_all_directory_for_test) {
    auto storage = Storage::getInstance();
    storage->deleteAllDirectory();

    BOOST_TEST(true);
  }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_TransactionPool)
//    BOOST_AUTO_TEST_CASE(push) {
//      TransactionPool transaction_pool;
//      Transaction transaction;
//      transaction_pool.push(transaction);
//
//      BOOST_CHECK_EQUAL(transaction_pool.size(), 1);
//    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_SignaturePool)
//  BOOST_AUTO_TEST_CASE(fetchN) {
//    SignaturePool signature_pool;
//    Signature signature1;
//    Signature signature2;
//
//    signature_pool.push(signature1);
//    signature_pool.push(signature2);
//
//    auto signatures = signature_pool.fetchN(2);
//
//    BOOST_CHECK_EQUAL(signatures.size(), 2);
//  }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_Storage_Service)

  BOOST_AUTO_TEST_CASE(find_latest_hash_and_height) {
    StorageFixture storage_fixture;
    pair<string, int> hash_and_height = storage_fixture.m_storage->findLatestHashAndHeight();

    BOOST_TEST(hash_and_height.first == Sha256::toString(hash_sample));
    BOOST_TEST(hash_and_height.second == 2);
  }

  BOOST_AUTO_TEST_CASE(find_latest_txid_list) {
    StorageFixture storage_fixture;
    vector<string> tx_ids_list = storage_fixture.m_storage->findLatestTxIdList();

    BOOST_TEST(tx_ids_list[0] == "Qa");
    BOOST_TEST(tx_ids_list[1] == "Qb");
    BOOST_TEST(tx_ids_list[2] == "Qc");
  }

  BOOST_AUTO_TEST_CASE(find_cert) {
    // 1번째로 등록된 certificates sample - a1 : 20171210~20181210, a2 : 20160303~20170303
    // 2번째로 등록된 certificates sample - a1 : 20180201~20190201, a2 : 20170505~20180505
    StorageFixture storage_fixture;
    auto certificate1 = storage_fixture.m_storage->findCertificate("a1"); // 최신(20180201에 등록) 인증서
    auto certificate2 = storage_fixture.m_storage->findCertificate("a2"); // 최신(20170505에 등록) 인증서
    auto certificate3 = storage_fixture.m_storage->findCertificate("a1", 1530409054); // 20180701
    auto certificate4 = storage_fixture.m_storage->findCertificate("a2", 1491874654); // 20170411

    BOOST_TEST(certificate1 == block_body_sample2["tx"][0]["content"][1].get<string>());
    BOOST_TEST(certificate2 == block_body_sample2["tx"][0]["content"][3].get<string>());
    BOOST_TEST(certificate3 == block_body_sample2["tx"][0]["content"][1].get<string>());
    BOOST_TEST(certificate4 == "");
  }

  BOOST_AUTO_TEST_CASE(read_block_for_block_processor) {
    StorageFixture storage_fixture;
    auto height_raw_tx = storage_fixture.m_storage->readBlock(1);
    auto latest_height_raw_tx = storage_fixture.m_storage->readBlock(-1);
    auto no_data_raw_tx = storage_fixture.m_storage->readBlock(9999);

    BOOST_TEST(1 == get<0>(height_raw_tx));
    BOOST_TEST(TypeConverter::toBase64Str(block_header_sample1.dump()) == get<1>(height_raw_tx));
    //cout << get<2>(height_raw_tx) << endl;

    BOOST_TEST(2 == get<0>(latest_height_raw_tx));
    BOOST_TEST(TypeConverter::toBase64Str(block_header_sample2.dump()) == get<1>(latest_height_raw_tx));
    //cout << get<2>(latest_height_raw_tx) <<endl;

    BOOST_TEST(-1 == get<0>(no_data_raw_tx));
    BOOST_TEST("" == get<1>(no_data_raw_tx));
    BOOST_TEST("" == get<2>(no_data_raw_tx));
  }

  BOOST_AUTO_TEST_CASE(find_sibling) {
    StorageFixture storage_fixture;
    vector<string> siblings = storage_fixture.m_storage->findSibling("c");
    if(!siblings.empty()){
      BOOST_TEST("h2" == siblings[0]);
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator_for_storage)
  BOOST_AUTO_TEST_CASE(save_block_by_block_object) {
    PartialBlock p_block;
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
    signer_id_type signer_id = TypeConverter::integerToBytes(1);
    signature.push_back({signer_id, TypeConverter::integerToBytes(1)});

    MerkleTree tree;
    vector<Transaction> transactions = {test_tx};
    tree.generate(transactions);

    BlockGenerator generator;
    generator.generateBlock(p_block, signature, tree);

    auto storage = Storage::getInstance();

    auto hash_and_height = storage->findLatestHashAndHeight();
    BOOST_CHECK_EQUAL(hash_and_height.second, 1);

    auto latest_list = storage->findLatestTxIdList();
    auto tx_id = latest_list[0];
    string encoded_tx_id = TypeConverter::toBase64Str(test_tx.getId());
    BOOST_CHECK_EQUAL(tx_id, encoded_tx_id);

    storage->deleteAllDirectory();
}

BOOST_AUTO_TEST_SUITE_END()