#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <typeinfo>

#include "../../src/modules/module.hpp"
#include "../../src/application.hpp"
#include "../../src/modules/communication/grpc_util.hpp"
#include "../../src/chain/transaction.hpp"
#include "../../src/modules/storage/storage.hpp"
#include "../../src/modules/storage/block_json.hpp"

using namespace std;
using namespace gruut;
using namespace nlohmann;

BOOST_AUTO_TEST_SUITE(Test_HeaderController)
    BOOST_AUTO_TEST_CASE(attchHeader) {
        MessageHeader msg_hdr;
        msg_hdr.identifier = 'G';
        msg_hdr.version = '1';
        msg_hdr.message_type = MessageType::MSG_ECHO;
        msg_hdr.mac_algo_type = MACAlgorithmType::RSA;
        msg_hdr.compression_algo_type = CompressionAlgorithmType::LZ4;
        msg_hdr.dummy = '1';

        string data("ggg");
        string header_added_data("G1123112341234567812345678123456ggg");
        header_added_data[2] = 0x58;
        header_added_data[3] = 0x00;
        header_added_data[4] = 0x04;
        header_added_data[6] = 0x00;
        header_added_data[7] = 0x00;
        header_added_data[8] = 0x00;
        header_added_data[9] = 0x23;

        BOOST_TEST(header_added_data ==
                   HeaderController::attachHeader(data, msg_hdr.message_type, msg_hdr.mac_algo_type, msg_hdr.compression_algo_type));
    }
    BOOST_AUTO_TEST_CASE(detachHeader) {
        string header_added_data = "11111111111111111111111111111111data";
        string data = "data";
        string header_detached_data = HeaderController::detachHeader(header_added_data);

        BOOST_TEST(header_detached_data==data);
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
    BOOST_AUTO_TEST_CASE(getJsonSize) {
        MessageHeader msg_hdr;
        msg_hdr.total_length[3] = 0x00;
        msg_hdr.total_length[2] = 0x00;
        msg_hdr.total_length[1] = 0x00;
        msg_hdr.total_length[0] = 0x41;

        BOOST_TEST(HeaderController::getJsonSize(msg_hdr) == (65 - HEADER_LENGTH));
    }
    BOOST_AUTO_TEST_CASE(parseHeader) {
        string header_added_data("G1123112341234567812345678123456ggg");
        header_added_data[2] = 0x58;
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
        for(int i=0; i<3; i++)
            compare_hdr.total_length[i] = 0x00;
        compare_hdr.total_length[3] = 0x23;
        for(int i=0; i<8; i++){
            compare_hdr.local_chain_id[i] = LOCAL_CHAIN_ID[i];
            compare_hdr.sender_id[i] = SENDER_ID[i];
        }
        for(int i=0; i<6; i++)
            compare_hdr.reserved_space[i] = RESERVED[i];

        MessageHeader origin_hdr = HeaderController::parseHeader(header_added_data);

        BOOST_TEST(origin_hdr.identifier == compare_hdr.identifier);
        BOOST_TEST(origin_hdr.version == compare_hdr.version);
        BOOST_TEST(static_cast<uint8_t>(origin_hdr.message_type) == static_cast<uint8_t>(compare_hdr.message_type));
        BOOST_TEST(static_cast<uint8_t>(origin_hdr.mac_algo_type) == static_cast<uint8_t>(compare_hdr.mac_algo_type));
        BOOST_TEST(static_cast<uint8_t>(origin_hdr.compression_algo_type) == static_cast<uint8_t>(compare_hdr.compression_algo_type));
        BOOST_TEST(origin_hdr.dummy == compare_hdr.dummy);
        BOOST_TEST(memcmp(origin_hdr.total_length, compare_hdr.total_length, 4)==0);
        BOOST_TEST(memcmp(origin_hdr.local_chain_id, compare_hdr.local_chain_id, 8)==0);
        BOOST_TEST(memcmp(origin_hdr.sender_id, compare_hdr.sender_id, 8)==0);
        BOOST_TEST(memcmp(origin_hdr.reserved_space, compare_hdr.reserved_space, 6)==0);
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
    BOOST_AUTO_TEST_CASE(write) {
        string tx_list;
        for (unsigned int idx = 0; idx < block["tx"].size() - 1; ++idx) {
            tx_list += block["tx"][idx]["txID"].dump();
            tx_list += '_';
        }
        block["block"]["txList"] = tx_list;

        json blk = block["block"];
        string blk_id = block["block"]["bID"];

        Storage storage;
        storage.openDB("./blkmeta");
        if(storage.write(blk, "blkmeta"))
            cout << "블록 메타 Object 저장 성공" << endl;
        else
            cout << "블록 메타 Object 저장 실패" << endl;
        storage.destroyDB();

        Storage storage2;
        storage2.openDB("./blkhash");
        if (storage.write(blk_id, BLKHASH))
            cout << "블록 해시 저장 성공" << endl;
        else
            cout << "블록 해시 저장 실패" << endl;
        storage2.destroyDB();

        Storage storage3;
        storage3.openDB("./blk");
        if (storage.write(blk, "block"))
            cout << "블록 Object 저장 성공" << endl;
        else
            cout << "블록 Object 저장 실패" << endl;
        storage3.destroyDB();

        Storage storage4;
        json tx_id = block["tx"];
        storage4.openDB("./secure");
        if (storage4.write(tx_id, "tx", blk_id))
            cout << "트랜잭션 Object 저장 성공" << endl;
        else
            cout << "트랜잭션 Object 저장 실패" << endl;
        storage4.destroyDB();

        Storage storage5;
        string n_id = cert["nid"];
        string x_509 = cert["x.509"];
        storage5.openDB("./cert");
        if (storage5.write(n_id, x_509))
            cout << "인증서 저장 성공" << endl;
        else
            cout << "인증서 저장 실패" << endl;
        storage5.destroyDB();

        BOOST_TEST(true);
    }

    BOOST_AUTO_TEST_CASE(findBlkMeta) {
        Storage st_blkmeta;
        st_blkmeta.openDB("./blkmeta");
        cout << st_blkmeta.find("ver").dump() << endl;
        cout << st_blkmeta.find("chainID").dump() << endl;
        cout << st_blkmeta.find("time").dump() << endl;
        cout << st_blkmeta.find("height").dump() << endl;
        cout << st_blkmeta.find("sSig").dump() << endl;
        cout << st_blkmeta.find("mID").dump() << endl;
        cout << st_blkmeta.find("mSig").dump() << endl;
        cout << st_blkmeta.find("pbID").dump() << endl;
        cout << st_blkmeta.find("prev").dump() << endl;
        cout << st_blkmeta.find("txCnt").dump() << endl;
        cout << st_blkmeta.find("txRt").dump() << endl;
        cout << st_blkmeta.find("txList").dump() << endl;

        st_blkmeta.destroyDB();
        BOOST_TEST(true);
    }

    BOOST_AUTO_TEST_CASE(findBlk) {
        Storage st_blk;
        st_blk.openDB("./blk");
        string blk_id = "3fffffffffffffffffff";
        cout << st_blk.find(blk_id, "ver").dump() << endl;
        cout << st_blk.find(blk_id, "chainID").dump() << endl;
        cout << st_blk.find(blk_id, "time").dump() << endl;
        cout << st_blk.find(blk_id, "height").dump() << endl;
        cout << st_blk.find(blk_id, "sSig").dump() << endl;
        cout << st_blk.find(blk_id, "mID").dump() << endl;
        cout << st_blk.find(blk_id, "mSig").dump() << endl;
        cout << st_blk.find(blk_id, "pbID").dump() << endl;
        cout << st_blk.find(blk_id, "prev").dump() << endl;
        cout << st_blk.find(blk_id, "txCnt").dump() << endl;
        cout << st_blk.find(blk_id, "txRt").dump() << endl;
        cout << st_blk.find(blk_id, "txList").dump() << endl;

        st_blk.destroyDB();
        BOOST_TEST(true);
    }
    BOOST_AUTO_TEST_CASE(findBlkHash) {
        Storage st_blk_hash;
        st_blk_hash.openDB("./blkhash");
        string blk_id = "3fffffffffffffffffff";
        auto result = st_blk_hash.find(blk_id);
        auto result_str = result.dump();
        cout<< result_str <<endl;

        st_blk_hash.destroyDB();
        BOOST_TEST(true);
    }

    BOOST_AUTO_TEST_CASE(findTx) {
        Storage st_tx;
        st_tx.openDB("./secure");
        string tx_id = "ddddddd";
        cout << st_tx.find(tx_id, "time").dump() << endl;
        cout << st_tx.find(tx_id, "rID").dump() << endl;
        cout << st_tx.find(tx_id, "rSig").dump() << endl;
        cout << st_tx.find(tx_id, "type").dump() << endl;
        cout << st_tx.find(tx_id, "content").dump() << endl;
        cout << st_tx.find(tx_id, "bID").dump() << endl;

        st_tx.destroyDB();
        BOOST_TEST(true);
    }

    BOOST_AUTO_TEST_CASE(findCert) {
        Storage st_cert;
        st_cert.openDB("./cert");
        string n_id = "aw98wueiwejnkwe";
        cout<< st_cert.find(n_id).dump()<<endl;

        st_cert.destroyDB();
        BOOST_TEST(true);
    }
BOOST_AUTO_TEST_SUITE_END()