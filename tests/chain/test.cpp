#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <botan-2/botan/hex.h>
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

        t.generate(transactions);

        auto merkle_tree = t.getMerkleTree();

        auto parent_digest = merkle_tree.back();
        auto parent_digest_hex = Botan::hex_encode(parent_digest);
        BOOST_CHECK_EQUAL(parent_digest_hex, "B7D05F875F140027EF5118A2247BBB84CE8F2F0F1123623085DAF7960C329F5F");
    }
BOOST_AUTO_TEST_SUITE_END()
