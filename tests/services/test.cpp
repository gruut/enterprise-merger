#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <vector>

#include "../../src/application.hpp"

#include "../../src/services/transaction_fetcher.hpp"
#include "../../src/services/signer_pool_manager.hpp"
#include "../../src/services/block_generator.hpp"
#include "../../src/services/message_factory.hpp"

#include "../../src/chain/transaction.hpp"
#include "../../src/chain/message.hpp"
#include "../../src/chain/types.hpp"

#include "../../include/nlohmann/json.hpp"

#include "../../src/utils/compressor.hpp"

using namespace gruut;
using namespace nlohmann;
using namespace std;

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator)

    BOOST_AUTO_TEST_CASE(generatePartialBlock) {
        vector<Transaction> transactions;
        transactions.push_back(Transaction());

        BlockGenerator generator;

        auto block = generator.generatePartialBlock(sha256());

        bool result = stoi(block.sent_time) > 0;
        BOOST_TEST(result);
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