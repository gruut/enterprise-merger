#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <botan/hex.h>
#include <utility>

#include "../../src/chain/merkle_tree.hpp"
#include "../../src/utils/type_converter.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_MerkleTree)
    BOOST_AUTO_TEST_CASE(generate_merkle_tree) {
        MerkleTree t;
        vector<sha256> transaction_ids;

        const string id = "1";
        auto transaction_id = Sha256::hash(id);

        transaction_ids.emplace_back(transaction_id);
        transaction_ids.emplace_back(transaction_id);
        transaction_ids.emplace_back(transaction_id);
        transaction_ids.emplace_back(transaction_id);
        t.generate(transaction_ids);

        auto merkle_tree = t.getMerkleTree();

        auto parent_digest = merkle_tree.back();
        auto parent_digest_hex = Botan::hex_encode(parent_digest);
        BOOST_CHECK_EQUAL(parent_digest_hex, "6C195BBC63D4168EFD589E6AEA788CE48F77DFD3F58F652A5E5923731970634A");
    }
BOOST_AUTO_TEST_SUITE_END()
