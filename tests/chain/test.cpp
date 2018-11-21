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

        MerkleTree t;
        vector<sha256> transaction_ids;
        transaction_ids.emplace_back("6B86B273FF34FCE19D6B804EFF5A3F5747ADA4EAA22F1D49C01E52DDB7875B4B"); // sha256(1)
        transaction_ids.emplace_back("D4735E3A265E16EEE03F59718B9B5D03019C07D8B6C51F90DA3A666EEC13AB35"); // sha256(2)
        transaction_ids.emplace_back("4E07408562BEDB8B60CE05C1DECFE3AD16B72230967DE01F640B7E4729B49FCE"); // sha256(3)
        transaction_ids.emplace_back("4B227777D4DD1FC61C6F884F48641D02B4D121D3FD328CB08B5531FCACDABF8A"); // sha256(4)
        transaction_ids.emplace_back("EF2D127DE37B942BAAD06145E54B0C619A1F22327B2EBBCFBEC78F5564AFE39D"); // sha256(5)
        transaction_ids.emplace_back("E7F6C011776E8DB7CD330B54174FD76F7D0216B612387A5FFCFB81E6F0919683"); // sha256(6)

        auto transaction_root = t.generate(transaction_ids);

        bool result = transaction_root == "63CD9C509AA5F6B9B3257123C01D2D1037797271BD4294CA74763F65E4B84812";
        BOOST_TEST(result);

        auto &tree = t.getTree();
        auto childs = *tree.find("63CD9C509AA5F6B9B3257123C01D2D1037797271BD4294CA74763F65E4B84812");
        BOOST_TEST(childs.second.first == "CC220774E4FA38B49110107C4DE38DF2C28328B00345E403EF415A577C476E90");
        BOOST_TEST(childs.second.second == "614FBDFCE3A9A8A500A369369FC98B3AB96F34E62A604B0D1FFA8C9611363066");
    }

BOOST_AUTO_TEST_SUITE_END()
