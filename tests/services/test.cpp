#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <vector>

#include "../../src/chain/transaction.hpp"
#include "../../src/chain/signature.hpp"
#include "../../src/chain/message.hpp"
#include "../../src/chain/types.hpp"

#include "nlohmann/json.hpp"

#include "../../src/services/block_generator.hpp"
#include "../../src/services/input_queue.hpp"
#include "../../src/services/output_queue.hpp"
#include "../../src/services/message_validator.hpp"

#include "../../src/utils/compressor.hpp"
#include "../../src/utils/type_converter.hpp"

#include "../../src/services/storage.hpp"
#include "block_json.hpp"

#include "fixture.hpp"

#include "../../src/application.hpp"

using namespace gruut;
using namespace std;

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator)

    BOOST_AUTO_TEST_CASE(generatePartialBlock) {
//        BlockGenerator generator;
//
//        auto tx_digest = Sha256::hash("1");
//
//        vector<Transaction> transactions;
//        transactions.emplace_back(Transaction());
//
//        auto block = generator.generatePartialBlock(tx_digest, transactions);
//        BOOST_TEST(true);
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
//      SignerPoolFixture signer_pool_fixture;
//      signer_pool_fixture.push();
//      BOOST_CHECK_EQUAL(signer_pool_fixture.signer_pool.size(), 1);
    }

    BOOST_AUTO_TEST_CASE(updateStatus) {
//      SignerPoolFixture signer_pool_fixture;
//      signer_pool_fixture.push();
//      signer_id_type prev_signer_id = TypeConverter::integerToBytes(signer_pool_fixture.id_int - 1);
//      signer_pool_fixture.signer_pool.updateStatus(prev_signer_id, SignerStatus::TEMPORARY);
//
//      auto signer = signer_pool_fixture.signer_pool.getSigner(0);
//      bool result = signer.status == SignerStatus::TEMPORARY;
//      BOOST_TEST(result);
    }

  BOOST_AUTO_TEST_CASE(getNumSignerBy) {
//    SignerPoolFixture signer_pool_fixture;
//    signer_pool_fixture.push();
//    signer_pool_fixture.push();
//    signer_pool_fixture.push();
//    signer_pool_fixture.push();
//
//    auto signers = signer_pool_fixture.signer_pool.getRandomSigners(2);
//    BOOST_CHECK_EQUAL(signers.size(), 2);
  }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_MessageIOQueues)

  BOOST_AUTO_TEST_CASE(pushMessages) {
    json msg_body = "{}"_json;
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

  BOOST_AUTO_TEST_CASE(create_transactions) {
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
  }

  BOOST_AUTO_TEST_CASE(delete_all_directory_for_test) {
    auto storage = Storage::getInstance();
  storage->destroyDB();

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
    pair<string, int> hash_and_height = storage_fixture.m_storage->getLatestHashAndHeight();

    BOOST_TEST(hash_and_height.first == TypeConverter::encodeBase64(block_hash_2));
    BOOST_TEST(hash_and_height.second == 2);
  }

  BOOST_AUTO_TEST_CASE(find_latest_txid_list) {
    StorageFixture storage_fixture;
    vector<string> tx_ids_list = storage_fixture.m_storage->getNthTxIdList();

    BOOST_TEST(tx_ids_list[0] == "Qa");
    BOOST_TEST(tx_ids_list[1] == "Qb");
    BOOST_TEST(tx_ids_list[2] == "Qc");
  }

  BOOST_AUTO_TEST_CASE(find_cert) {
    // 1번째로 등록된 certificates sample - a1 : 20171210~20181210, a2 : 20160303~20170303
    // 2번째로 등록된 certificates sample - a1 : 20180201~20190201, a2 : 20170505~20180505
//    StorageFixture storage_fixture;
//    auto certificate1 = storage_fixture.m_storage->getCertificate("a1"); // 최신(20180201에 등록) 인증서
//    auto certificate1 =
//    auto certificate2 = storage_fixture.m_storage->getCertificate("a2"); // 최신(20170505에 등록) 인증서
//    auto certificate3 = storage_fixture.m_storage->getCertificate("a1", 1530409054); // 20180701
//    auto certificate4 = storage_fixture.m_storage->getCertificate("a2", 1491874654); // 20170411
//
//    BOOST_TEST(certificate1 == block_body_sample2["tx"][0]["content"][1].get<string>());
//    BOOST_TEST(certificate2 == block_body_sample2["tx"][0]["content"][3].get<string>());
//    BOOST_TEST(certificate3 == block_body_sample2["tx"][0]["content"][1].get<string>());
//    BOOST_TEST(certificate4 == "");
  }

//  BOOST_AUTO_TEST_CASE(read_block_for_block_processor) {
//    StorageFixture storage_fixture;
//    auto height_raw_tx = storage_fixture.m_storage->readBlock(1);
//    auto latest_height_raw_tx = storage_fixture.m_storage->readBlock(0);
//    auto no_data_raw_tx = storage_fixture.m_storage->readBlock(9999);
//
//    BOOST_TEST(1 == get<0>(height_raw_tx));
//    BOOST_TEST(block_raw_sample1_bytes == get<1>(height_raw_tx));
//    //cout << get<2>(height_raw_tx) << endl;
//
//    BOOST_TEST(2 == get<0>(latest_height_raw_tx));
//    BOOST_TEST(block_raw_sample2_bytes == get<1>(latest_height_raw_tx));
//    //cout << get<2>(latest_height_raw_tx) <<endl;
//
//    BOOST_TEST(0 == get<0>(no_data_raw_tx));
//    BOOST_TEST(get<1>(no_data_raw_tx).empty());
//    BOOST_TEST(get<2>(no_data_raw_tx).empty());
//  }

  BOOST_AUTO_TEST_CASE(find_sibling) {
    StorageFixture storage_fixture;
    proof_type proof = storage_fixture.m_storage->getProof("c");

    if(!proof.block_id_b64.empty() || !proof.siblings.empty()){

      BOOST_TEST(proof.siblings[0].first == false);
      BOOST_TEST(proof.siblings[0].second == "U22Yg38t0WWlXV7q6RSFlURy1W8kbfJWvzyuGTUqEjw=");

      std::string root_val_b64 = TypeConverter::encodeBase64(storage_fixture.m_mtree_root_1);

      BOOST_TEST(MerkleTree::isValidSiblings(proof,root_val_b64));

    } else
      BOOST_TEST("EMPTY");
  }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator_for_storage)
  BOOST_AUTO_TEST_CASE(save_block_by_block_object) {
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

    BlockGenerator generator;
    generator.generateBlock(p_block, signature, tree);

    auto storage = Storage::getInstance();

    auto hash_and_height = storage->getLatestHashAndHeight();
    BOOST_CHECK_EQUAL(hash_and_height.second, 1);

    auto latest_list = storage->getNthTxIdList();
    auto tx_id = latest_list[0];
    string encoded_tx_id = TypeConverter::encodeBase64(test_tx.getId());
    BOOST_CHECK_EQUAL(tx_id, encoded_tx_id);

  storage->destroyDB();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_MessageValidator)
  BOOST_AUTO_TEST_CASE(validationSuccess) {

    InputMsgEntry msg_join_test;
    json msg_join_json = R"({
      "sID": "UAABACACAAE=",
      "time": "1543323592",
      "ver": "1020181127",
      "cID": "AAAAAAAAAAE="
    })"_json;
    msg_join_test.type = MessageType::MSG_JOIN;
    msg_join_test.body = msg_join_json;

    InputMsgEntry msg_response_1_test;
    json msg_response_1_json = R"({
      "sID": "UAABACACAAE=",
      "time": "1543323592",
      "cert": "-----BEGIN CERTIFICATE-----\nMIIDLDCCAhQCBgEZlK1CPjA....\n-----END CERTIFICATE-----",
      "sN": "luLSQgVDnMyZjkAh8h2HNokaY1Oe9Md6a4VpjcdGgzs=",
      "dhx": "92943e52e02476bd1a4d74c2498db3b01c204f29a32698495b4ed0a274e12294",
      "dhy": "96e2d24205439ccc998e4021f21d8736891a63539ef4c77a6b85698dc746833b",
      "sig": "QWVMP1UfUJIemaLFqnXvQfGqghVCmYH0yXo1/g5hUWAbouuXdTI/O7Gkgz3C5kXhnIWZ+dHp...."
    })"_json;
    msg_response_1_test.type = MessageType::MSG_RESPONSE_1;
    msg_response_1_test.body = msg_response_1_json;

    InputMsgEntry msg_success_test;
    json msg_success_json = R"({
      "sID": "UAABACACAAE=",
      "time": "1543323592",
      "val": true
    })"_json;
    msg_success_test.type = MessageType::MSG_SUCCESS;
    msg_success_test.body = msg_success_json;

    InputMsgEntry msg_tx_test;
    json msg_tx_json = R"({
      "txid": "Sv0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg=",
      "time": "1543323592",
      "rID": "AAAAAAAAAAE=",
      "type": "CERTIFICATES",
      "content": [
          "UAABACACAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
          "UAABACACEAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
          "UAABACCCAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----"
      ],
      "rSig": "SM49AwEHA0IABBXqXQz72KPYPp9lwiQeqKaO9MF313icIj8GC2Il0Cy6QuDAiEC....."
    })"_json;
    msg_tx_test.type = MessageType::MSG_TX;
    msg_tx_test.body = msg_tx_json;

    InputMsgEntry msg_ssig_test;
    json msg_ssig_json = R"({
      "sID": "UAABACACAAE=",
      "time": "1543323592",
      "sig": "QWVMP1UfUJIemaLFqnXvQfGqghVCmYH0yXo1/g5hUWAbouuXdTI/O7Gkgz3C5kXhnIWZ+dHp...."
    })"_json;
    msg_ssig_test.type = MessageType::MSG_SSIG;
    msg_ssig_test.body = msg_ssig_json;

    InputMsgEntry msg_ping_test;
    json msg_ping_json = R"({
      "mID": "AAAAAAAAAAE=",
      "time": "1543323592",
      "sCnt": "25",
      "stat": "p"
    })"_json;
    msg_ping_test.type = MessageType::MSG_PING;
    msg_ping_test.body = msg_ping_json;

    InputMsgEntry msg_up_test;
    json msg_up_json = R"({
      "mID": "AAAAAAAAAAE=",
      "time": "1543323592",
      "ver": "1020181127",
      "cID": "AAAAAAAAAAE="
    })"_json;
    msg_up_test.type = MessageType::MSG_UP;
    msg_up_test.body = msg_up_json;

    InputMsgEntry msg_req_check_test;
    json msg_req_check_json = R"({
      "sender": "AAAAAAAAAAE=",
      "time": "1543323592",
      "dID": "AAAAAAAAAAE=",
      "txid": "Sv0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg="
    })"_json;
    msg_req_check_test.type = MessageType::MSG_REQ_CHECK;
    msg_req_check_test.body = msg_req_check_json;

    InputMsgEntry msg_req_header_test;
    json msg_req_header_json = R"({
      "rID": "AAAAAAAAAAE=",
      "time": "1543323592",
      "rCert": "-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
      "hgt": "1",
      "rSig": "Sv0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg="
    })"_json;
    msg_req_header_test.type = MessageType::MSG_REQ_HEADER;
    msg_req_header_test.body = msg_req_header_json;

    InputMsgEntry msg_block_test;
    json msg_block_json = R"({
      "mID": "AAAAAAAAAAE=",
      "blockraw": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
      "tx":[
        {
          "txid": "Sv0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg=",
          "time": "1543323592",
          "rID": "AAAAAAAAAAE=",
          "type": "CERTIFICATES",
          "content": [
            "UAABACACAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
            "UAABACACEAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
            "UAABACCCAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----"
          ],
          "rSig": "SM49AwEHA0IABBXqXQz72KPYPp9lwiQeqKaO9MF313icIj8GC2Il0Cy6QuDAiEC....."
        },
        {
          "txid": "Av0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg=",
          "time": "1543323592",
          "rID": "AAAAAAAAAAE=",
          "type": "CERTIFICATES",
          "content": [
            "UAABACACAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
            "UAABACACEAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
            "UAABACCCAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----"
          ],
          "rSig": "SM49AwEHA0IABBXqXQz72KPYPp9lwiQeqKaO9MF313icIj8GC2Il0Cy6QuDAiEC....."
        }
      ]
    })"_json;
    msg_block_test.type = MessageType::MSG_BLOCK;
    msg_block_test.body = msg_block_json;

    InputMsgEntry msg_req_block_test;
    json msg_req_block_json = R"({
      "mID": "AAAAAAAAAAE=",
      "time": "1543323592",
      "mCert": "-----BEGIN CERTIFICATE-----\nMIIDLDCCAhQCBgEZlK1CPjA....\n-----END CERTIFICATE-----",
      "hgt": "1",
      "mSig": "vQPcP1sloKtQzEP+cOgYwbf0F3QhblPsABOtJrq8nNAEXtibe5J/9B4d4t920JLnQsbZtSMHo...."
    })"_json;
    msg_req_block_test.type = MessageType::MSG_REQ_BLOCK;
    msg_req_block_test.body = msg_req_block_json;

    MessageValidator validator_test;

    BOOST_CHECK_EQUAL(validator_test.validate(msg_join_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_response_1_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_success_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_tx_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_ssig_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_ping_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_up_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_req_check_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_block_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_req_block_test), true);
    BOOST_CHECK_EQUAL(validator_test.validate(msg_req_header_test), true);
  }

BOOST_AUTO_TEST_CASE(validationFailure) {
  MessageValidator validator_test;

  // sID entry가 빠진 경우
  InputMsgEntry msg_join_test;
  json msg_join_json = R"({
        "time": "1543323592",
        "ver": "1020181127",
        "cID": "AAAAAAAAAAE="
      })"_json;
  msg_join_test.type = MessageType::MSG_JOIN;
  msg_join_test.body = msg_join_json;

  BOOST_CHECK_EQUAL(validator_test.validate(msg_join_test), false);

  // signer nonce 길이가 다른 경우
  InputMsgEntry msg_response_1_test;
  json msg_response_1_json = R"({
      "sID": "UAABACACAAE=",
      "time": "1543323592",
      "cert": "-----BEGIN CERTIFICATE-----\nMIIDLDCCAhQCBgEZlK1CPjA....\n-----END CERTIFICATE-----",
      "sN": "abcd",
      "dhx": "92943e52e02476bd1a4d74c2498db3b01c204f29a32698495b4ed0a274e12294",
      "dhy": "96e2d24205439ccc998e4021f21d8736891a63539ef4c77a6b85698dc746833b",
      "sig": "QWVMP1UfUJIemaLFqnXvQfGqghVCmYH0yXo1/g5hUWAbouuXdTI/O7Gkgz3C5kXhnIWZ+dHp...."
    })"_json;
  msg_response_1_test.type = MessageType::MSG_RESPONSE_1;
  msg_response_1_test.body = msg_response_1_json;

  BOOST_CHECK_EQUAL(validator_test.validate(msg_response_1_test), false);

  // content가 잘못 입력됐을 경우
  InputMsgEntry msg_tx_test;
  json msg_tx_json = R"({
        "txid": "Sv0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg=",
        "time": "1543323592",
        "rID": "AAAAAAAAAAE=",
        "type": "CERTIFICATES",
        "content": [
            "UAABACACAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
            "UAABACACEAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----",
            "UAABACCCAAE=","-----BEGIN CERTIFICATE-----\n....\n-----END CERTIFICATE-----"
        ],
        "rSig": "SM49AwEHA0IABBXqXQz72KPYPp9lwiQeqKaO9MF313icIj8GC2Il0Cy6QuDAiEC....."
      })"_json;

  msg_tx_json["content"] = "UAABACACAAE=";
  msg_tx_test.type = MessageType::MSG_TX;
  msg_tx_test.body = msg_tx_json;

  BOOST_CHECK_EQUAL(validator_test.validate(msg_tx_test), false);

  // signer count가 200을 초과 했을 경우
  InputMsgEntry msg_ping_test;
  json msg_ping_json = R"({
        "mID": "AAAAAAAAAAE=",
        "time": "1543323592",
        "sCnt": "201",
        "stat": "p"
      })"_json;

  msg_ping_test.type = MessageType::MSG_PING;
  msg_ping_test.body = msg_ping_json;

  BOOST_CHECK_EQUAL(validator_test.validate(msg_ping_test), false);

  // tx entry가 빠진 경우
  InputMsgEntry msg_block_test;
  json msg_block_json = R"({
        "mID": "AAAAAAAAAAE=",
        "blockraw": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      })"_json;
  msg_block_test.type = MessageType::MSG_BLOCK;
  msg_block_test.body = msg_block_json;

  BOOST_CHECK_EQUAL(validator_test.validate(msg_block_test), false);

  // hgt = -1
  InputMsgEntry msg_req_block_test;
  json msg_req_block_json = R"({
        "mID": "AAAAAAAAAAE=",
        "time": "1543323592",
        "mCert": "-----BEGIN CERTIFICATE-----\nMIIDLDCCAhQCBgEZlK1CPjA....\n-----END CERTIFICATE-----",
        "hgt": "-1",
        "mSig": "vQPcP1sloKtQzEP+cOgYwbf0F3QhblPsABOtJrq8nNAEXtibe5J/9B4d4t920JLnQsbZtSMHo...."
      })"_json;
  msg_req_block_test.type = MessageType::MSG_REQ_BLOCK;
  msg_req_block_test.body = msg_req_block_json;
  BOOST_CHECK_EQUAL(validator_test.validate(msg_req_block_test), false);

}

BOOST_AUTO_TEST_SUITE_END()