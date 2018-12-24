#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP

#include "../chain/block.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include <vector>

#include "signature_requester.hpp"

using namespace std;

namespace gruut {
class BlockGenerator {
public:
  PartialBlock generatePartialBlock(sha256 &merkle_root, vector<Transaction> &);
  // argument must be call-by-value due to multi-thread safe
  void generateBlock(PartialBlock partial_block, vector<Signature> support_sigs,
                     MerkleTree merkle_tree);
};
} // namespace gruut

#endif
