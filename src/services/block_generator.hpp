#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP

#include "../chain/block.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include <vector>

#include "signature_requester.hpp"

using namespace std;

namespace gruut {
using Transactions = vector<Transaction>;
class BlockGenerator {
public:
  PartialBlock generatePartialBlock(vector<sha256> &transactions_digest);
};
} // namespace gruut

#endif
