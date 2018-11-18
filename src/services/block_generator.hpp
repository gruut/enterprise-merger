#ifndef GRUUT_HANA_MERGER_BLOCK_GENERATOR_HPP
#define GRUUT_HANA_MERGER_BLOCK_GENERATOR_HPP

#include <vector>
#include "../chain/block.hpp"
#include "../chain/transaction.hpp"

#include "signature_requester.hpp"

using namespace std;

namespace gruut {
    using Transactions = vector<Transaction>;
    class BlockGenerator {
    public:
        PartialBlock generatePartialBlock(Transactions transactions);
    };
}

#endif
