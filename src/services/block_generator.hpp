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
  PartialBlock generatePartialBlock(vector<sha256> &transactions_digest,
                                    vector<Transaction> &);
  void generateBlock(PartialBlock &partial_block, vector<Signature> &signatures,
                     MerkleTree &merkle_tree);
  void toJson(json &j, const Transaction &tx);
};
} // namespace gruut

#endif
