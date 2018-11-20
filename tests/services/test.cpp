#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <vector>

#include "../../src/application.hpp"

#include "../../src/services/message_fetcher.hpp"
#include "../../src/services/transaction_fetcher.hpp"
#include "../../src/services/signer_pool_manager.hpp"
#include "../../src/services/signature_requester.hpp"
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

BOOST_AUTO_TEST_SUITE(Test_MessageFetcher)

    BOOST_AUTO_TEST_CASE(fetch) {
        auto transaction = MessageFetcher::fetch<Transaction>();
        string type_name = typeid(transaction).name();
        bool result = type_name.find("Transaction") != string::npos;
        BOOST_TEST(result);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_TransactionFetcher)

    BOOST_AUTO_TEST_CASE(fetchAll) {
        vector<Signer> signers;
        Signer signer;
        signer.address = "123";
        signer.cert = "123";

        signers.push_back(signer);
        TransactionFetcher tf(std::move(signers));
        auto transactions = tf.fetchAll();

        auto result = transactions.front().transaction_type == TransactionType::CERTIFICATE;
        BOOST_TEST(result);

        //  check transaction_id_size
        auto transaction = transactions.front();
        result = transaction.transaction_id.size() == 32;
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

    BOOST_AUTO_TEST_CASE(getSelectedSigners) {
            SignerPoolManager manager;

            auto signers = manager.getSigners();
            BOOST_TEST(signers.size() == 0);

            Signer signer;
            signer.cert = "1234";
            signer.address = "1234";

            manager.putSigner(std::move(signer));

            BOOST_TEST(manager.getSelectedSigners().size() == 0);

            manager.getSigners();

            BOOST_TEST(manager.getSelectedSigners().size() == 1);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_SignatureRequester)

    BOOST_AUTO_TEST_CASE(requestSignatures) {
        SignatureRequester requester;

        auto result = requester.requestSignatures();
        BOOST_TEST(result);

        result = Application::app().getOutputQueue()->size() > 0;
        BOOST_TEST(result);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_BlockGenerator)

    BOOST_AUTO_TEST_CASE(generatePartialBlock) {
        vector<Transaction> transactions;
        transactions.push_back(Transaction());

        BlockGenerator generator;

        auto block = generator.generatePartialBlock("1");

        bool result = stoi(block.sent_time) > 0;
        BOOST_TEST(result);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_MessageFactory)

    BOOST_AUTO_TEST_CASE(createSigRequestMessage) {
        PartialBlock block;
        MessageHeader message_header;
        auto message = MessageFactory::createSigRequestMessage(block);

        bool result = message.mac_algo_type == MACAlgorithmType::RSA;
        BOOST_TEST(result);

        json j_string2;
        j_string2["cID"] = "";
        j_string2["hgt"] = "";
        j_string2["mID"] = "";
        j_string2["time"] = "";
        j_string2["txrt"] = "";

        result = message.data == j_string2;
        BOOST_TEST(result);
    }

BOOST_AUTO_TEST_SUITE_END()