#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include "../../../src/modules/module.hpp"
#include "../../../src/application.hpp"
#include "../../../src/modules/message_fetcher/message_fetcher.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_MessageFetcher)
    BOOST_AUTO_TEST_CASE(start) {
        unique_ptr<MessageFetcher>p_msg_fetcher(new MessageFetcher());
        p_msg_fetcher->start();
        BOOST_TEST(true);
    }
BOOST_AUTO_TEST_SUITE_END()
