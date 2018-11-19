#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <typeinfo>

#include "../../src/modules/module.hpp"
#include "../../src/application.hpp"
#include "../../../src/modules/communication/grpc_util.hpp"
#include "../../src/chain/transaction.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_HeaderController)
    BOOST_AUTO_TEST_CASE(attchHeader) {
        string data = "1234";
        string header_added_data = HeaderController::attachHeader(data, MessageType::MSG_BLOCK);

        uint32_t total_length = data.size()+HEADER_LENGTH;
        string header;
        header.resize(32);
        header[0] = G;
        header[1] = VERSION;
        header[2] = static_cast<uint8_t>(MessageType::MSG_BLOCK);
        header[3] = MAC;
        header[4] = COMPRESSION_TYPE;
        header[5] = NOT_USED;
        for(int i=9; i>6; i--) {
            header[i]|=total_length;
            total_length = (total_length>>8);
        }
        header[6] |= total_length;

        uint64_t local_chain_id = LOCAL_CHAIN_ID;
        for(int i=17; i>10; i--){
            header[i]|=local_chain_id;
            local_chain_id =(local_chain_id>>8);
        }
        header[10] |=local_chain_id;

        uint64_t sender_id = SENDER;
        for(int i=25; i>18; i--){
            header[i]|=sender_id;
            sender_id=(sender_id>>8);
        }
        header[18] |= sender_id;

        for(int i=0; i<6; i++) {
            header[26 + i] = RESERVED[i];
        }
        BOOST_TEST(header_added_data==header+data);
    }
    BOOST_AUTO_TEST_CASE(detachHeader) {
        string header_added_data = "11111111111111111111111111111111data";
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
        header.resize(10);
        int total_length = 50;
        header[0]=G;
        header[1]=VERSION;
        header[2]=0x58;
        header[3]=MAC;
        header[4]=COMPRESSION_TYPE;
        header[5] = NOT_USED;
        for(int i=9; i>6; i--) {
            header[i]|=total_length;
            total_length = (total_length>>8);
        }
        header[6] |= total_length;

        BOOST_TEST(HeaderController::getJsonSize(header)==18);
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
    BOOST_AUTO_TEST_CASE(getCompressionType)
    {
        string header;
        header.resize(10);
        int total_length = 50;
        header[0]=G;
        header[1]=VERSION;
        header[2]=0x58;
        header[3]=MAC;
        header[4]=COMPRESSION_TYPE;
        header[5] = NOT_USED;
        uint8_t compress_type = HeaderController::getCompressionType(header);

        BOOST_TEST(compress_type == COMPRESSION_TYPE);
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