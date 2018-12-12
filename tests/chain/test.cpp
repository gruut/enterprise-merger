#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <botan/hex.h>
#include <utility>

#include "../../src/chain/merkle_tree.hpp"
#include "../../src/chain/transaction.hpp"
#include "../../src/utils/type_converter.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_MerkleTree)
    BOOST_AUTO_TEST_CASE(generate_merkle_tree) {
        MerkleTree t;
        vector<Transaction> transactions;

        transactions.emplace_back(Transaction());
        transactions.emplace_back(Transaction());
        transactions.emplace_back(Transaction());
        transactions.emplace_back(Transaction());
        t.generate(transactions);

        auto merkle_tree = t.getMerkleTree();

        auto parent_digest = merkle_tree.back();
        auto parent_digest_hex = Botan::hex_encode(parent_digest);
        BOOST_CHECK_EQUAL(parent_digest_hex, "58279BE4DAC761BAA230F81672BA8B69906D426A9312CA1FE0626AC13C35DB79");
    }
BOOST_AUTO_TEST_SUITE_END()
