#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>

#include "../../src/services/transaction_fetcher.hpp"
#include "../../src/chain/types.hpp"

using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_TransactionFetcher)
    BOOST_AUTO_TEST_CASE(fetch) {
        Signer signer;
        signer.address = "123";
        signer.cert = "123";

        TransactionFetcher tf(signer);
        auto transaction = tf.fetch();

        auto result = transaction.transaction_type == TransactionType::CERTIFICATE;
        BOOST_TEST(result);
    }
BOOST_AUTO_TEST_SUITE_END()
