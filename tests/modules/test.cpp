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