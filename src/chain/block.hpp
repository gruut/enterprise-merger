#ifndef GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP

#include "merkle_tree.hpp"
#include "signature.hpp"
#include "transaction.hpp"
#include "types.hpp"

#include <vector>

using namespace std;

namespace gruut {
struct PartialBlock {
  merger_id_type merger_id;
  local_chain_id_type chain_id;
  block_height_type height;
  transaction_root_type transaction_root;

  vector<Transaction> transactions;
};

struct Block : public PartialBlock {
  Block() = delete;
  Block(Block &) = delete;
  Block(Block &&) = default;
  Block operator=(Block &) = delete;

  Block(PartialBlock &partial_block) : PartialBlock(partial_block) {}

  // Meta
  CompressionAlgorithmType compression_algo_type;
  header_length_type header_length;

  // Header
  block_version_type version;
  block_header_hash_type previous_header_hash;
  block_id_type previous_block_id;
  block_id_type block_id;
  timestamp_type timestamp;
  vector<transaction_id_type> transaction_ids;
  vector<Signature> signer_signatures;
  signature_type signature;

  // Body
  MerkleTree merkle_tree;
  size_t transactions_count;
};
} // namespace gruut
#endif
