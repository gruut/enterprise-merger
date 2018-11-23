#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <utility>

#include "../../src/chain/merkle_tree.hpp"

using namespace std;
using namespace gruut;

BOOST_AUTO_TEST_SUITE(Test_MerkleTree)

    BOOST_AUTO_TEST_CASE(generate_merkle_tree) {

        /* Validates Merkle tree
                            63CD9C
                              |
                CC2207                  614FBD
                /      \                /     \                                                                                                                                                                                                                                  |
           F5FC       8BEF56        6C2B76     6C2B76
           / \         /  \          / \
    6B86B2 D4735E 4E0740 4B2277 EF2D12 E7F6C0
        */
//
//        MerkleTree t;
//        vector<sha256> transaction_ids;
//        transaction_ids.emplace_back("1");
//        transaction_ids.emplace_back("2");
//        transaction_ids.emplace_back("3");
//        transaction_ids.emplace_back("4");
//        transaction_ids.emplace_back("5");
//        transaction_ids.emplace_back("6");
//
//        auto transaction_root = t.generate(transaction_ids);

//        bool result = transaction_root == "63CD9C509AA5F6B9B3257123C01D2D1037797271BD4294CA74763F65E4B84812";
//        BOOST_TEST(true);
//
//        auto &tree = t.getTree();
//        auto childs = *tree.find("63CD9C509AA5F6B9B3257123C01D2D1037797271BD4294CA74763F65E4B84812");
//        BOOST_TEST(childs.second.first == "CC220774E4FA38B49110107C4DE38DF2C28328B00345E403EF415A577C476E90");
//        BOOST_TEST(childs.second.second == "614FBDFCE3A9A8A500A369369FC98B3AB96F34E62A604B0D1FFA8C9611363066");
    }

BOOST_AUTO_TEST_SUITE_END()
