#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP

#include "../chain/block.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/signature.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

#include "message_proxy.hpp"
#include "signature_requester.hpp"

#include <vector>

using namespace std;

namespace gruut {
class BlockGenerator {
public:
  BlockGenerator();
  PartialBlock generatePartialBlock(sha256 &merkle_root, vector<Transaction> &);
  // argument must be call-by-value due to multi-thread safe
  void generateBlock(PartialBlock partial_block, vector<Signature> support_sigs,
                     MerkleTree merkle_tree);
};
} // namespace gruut

#endif
