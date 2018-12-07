#pragma once

#include <vector>

#include "../config/config.hpp"
#include "../utils/sha256.hpp"
#include "types.hpp"

using namespace std;
using namespace gruut::config;

namespace gruut {
class MerkleTree {
public:
  MerkleTree() { m_merkle_tree.resize(MAX_MERKLE_LEAVES * 2 - 1); }

  void generate(vector<sha256> &tx_digest) {
    const bytes dummy_leaf(32, 0); // for SHA-256

    auto min_addable_size = min(MAX_MERKLE_LEAVES, tx_digest.size());
    // copying with padding
    for (size_t i = 0; i < min_addable_size; ++i) {
      m_merkle_tree[i] = tx_digest[i];
    }
    for (size_t i = min_addable_size; i < MAX_MERKLE_LEAVES; ++i)
      m_merkle_tree[i] = dummy_leaf;

    size_t parent_pos = MAX_MERKLE_LEAVES;
    for (size_t i = 0; i < MAX_MERKLE_LEAVES * 2 - 3; i += 2) {
      m_merkle_tree[parent_pos] =
          makeParent(m_merkle_tree[i], m_merkle_tree[i + 1]);
      ++parent_pos;
    }
  }

  vector<sha256> getMerkleTree() { return m_merkle_tree; }

private:
  sha256 makeParent(sha256 left, sha256 right) {
    left.insert(left.cend(), right.cbegin(), right.cend());
    sha256 hashed_bytes = Sha256::hash(left);

    return hashed_bytes;
  }

  vector<sha256> m_merkle_tree;
};
} // namespace gruut