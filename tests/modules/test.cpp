#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <typeinfo>

#include "../../src/modules/module.hpp"
#include "../../src/application.hpp"
#include "../../src/modules/message_fetcher/message_fetcher.hpp"
#include "../../src/modules/signer_pool_manager/signer_pool_manager.hpp"
#include "../../src/modules/signature_requester/signature_requester.hpp"
#include "../../../src/modules/communication/grpc_util.hpp"
#include "../../src/chain/transaction.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_MessageFetcher)
    BOOST_AUTO_TEST_CASE(start) {
        auto transaction = MessageFetcher::fetch<Transaction>();
        string type_name = typeid(transaction).name();
        bool result = type_name.find("Transaction") != string::npos;
        BOOST_TEST(result);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_SignerPoolManager)
    BOOST_AUTO_TEST_CASE(getSigners) {
        SignerPoolManager manager;

        auto signers = manager.getSigners();
        BOOST_TEST(signers.size() == 0);

        Signer signer;
        signer.cert = "1234";
        signer.address = "1234";

        manager.putSigner(std::move(signer));

        signers = manager.getSigners();
        BOOST_TEST(signers.size() == 1);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_SignatureRequester)
    BOOST_AUTO_TEST_CASE(requestSignatures) {
        SignatureRequester requester;

        auto result = requester.requestSignatures();
        BOOST_TEST(result);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_Compressor)
    BOOST_AUTO_TEST_CASE(compressData_decompressData) {
        string original = "2013-01-07 00:00:04,0.98644,0.98676 2013-01-07 00:01:19,0.98654,0.98676 2013-01-07 00:01:38,0.98644,0.98696";
        int origin_size = original.size();
        string compressed_data, decompressed_data;

        Compressor::compressData(original,compressed_data);
        Compressor::decompressData(compressed_data, decompressed_data, origin_size);

        BOOST_TEST(decompressed_data==original);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_HeaderController)
    BOOST_AUTO_TEST_CASE(attchHeader) {
        string data = "1234";
        string header_added_data = HeaderController::attachHeader(data, MessageType::MSG_BLOCK);

        uint32_t total_length = data.size()+HEADER_LENGTH;
        string header;
        header.resize(26);
        header[0]=G;
        header[1]=VERSION;
        header[2]= static_cast<u_int8_t>(MessageType::MSG_BLOCK);
        header[3]=MAC;
        for(int i=7; i>4; i--) {
            header[i]|=total_length;
            total_length = (total_length>>8);
        }
        header[4] |= total_length;

        uint64_t local_chain_id = LOCAL_CHAIN_ID;
        for(int i=15; i>8; i--){
            header[i]|=local_chain_id;
            local_chain_id =(local_chain_id>>8);
        }
        header[8] |=local_chain_id;

        uint64_t sender_id = SENDER;
        for(int i=23; i>16; i--){
            header[i]|=sender_id;
            sender_id=(sender_id>>8);
        }
        header[15] |= sender_id;

        uint16_t reserved = RESERVED;
        header[25] |= reserved;
        reserved = (reserved>>8);
        header[24] |=reserved;

        BOOST_TEST(header_added_data==header+data);
    }
    BOOST_AUTO_TEST_CASE(detachHeader) {
        string header_added_data = "11111111111111111111111111data";
        string data = "data";
        string header_detached_data = HeaderController::detachHeader(header_added_data);

        BOOST_TEST(header_detached_data==data);
    }
    BOOST_AUTO_TEST_CASE(validateMessage) {
        string header;
        header+=G;
        header+=VERSION;
        header+=0x58;
        header+=MAC;

        BOOST_TEST(HeaderController::validateMessage(header));
    }
    BOOST_AUTO_TEST_CASE(getJsonSize) {
        string header;
        int total_length = 50;
        header+=G;
        header+=VERSION;
        header+=0x58;
        header+=MAC;
        for(int i=7; i>4; i--) {
            header[i]|=total_length;
            total_length = (total_length>>8);
        }
        header[4] |= total_length;

        BOOST_TEST(HeaderController::getJsonSize(header)==24);
    }
    BOOST_AUTO_TEST_CASE(getMessageType) {
        string header;
        int total_length = 50;
        header+=G;
        header+=VERSION;
        header+=0x58;
        header+=MAC;
        uint8_t msg_type = HeaderController::getMessageType(header);

        BOOST_TEST(msg_type == static_cast<uint8_t>(MessageType::MSG_ECHO));
    }
BOOST_AUTO_TEST_SUITE_END()