#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include "../../src/modules/module.hpp"
#include "../../src/application.hpp"
#include "../../src/modules/message_fetcher/message_fetcher.hpp"
#include "../../src/modules/signer_pool_manager/signer_pool_manager.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_MessageFetcher)
    BOOST_AUTO_TEST_CASE(start) {
        unique_ptr<MessageFetcher>p_msg_fetcher(new MessageFetcher());
        p_msg_fetcher->start();
        BOOST_TEST(true);
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