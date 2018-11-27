#pragma once

#include <cstdint>
#include <vector>

#include <botan/hash.h>

namespace gruut {

constexpr size_t MAX_MERKLE_LEAVES = 4096;

class StaticMerkleTree {

public:
  std::vector<std::vector<uint8_t>>
  generate(std::vector<std::vector<uint8_t>> &tx_digest) {
    std::vector<uint8_t> dummy_leaf(32, 0); // for SHA-256

    std::vector<std::vector<uint8_t>> static_merkle_tree(MAX_MERKLE_LEAVES * 2 -
                                                         1);

    // copying with padding
    for (size_t i = 0; i < std::min(MAX_MERKLE_LEAVES, tx_digest.size()); ++i) {
      static_merkle_tree[i] = tx_digest[i];
    }
    for (size_t i = std::min(MAX_MERKLE_LEAVES, tx_digest.size());
         i < MAX_MERKLE_LEAVES; ++i) {
      static_merkle_tree[i] = dummy_leaf;
    }

    size_t parent_pos = MAX_MERKLE_LEAVES;
    for (size_t i = 0; i < MAX_MERKLE_LEAVES * 2 - 3; i += 2) {
      static_merkle_tree[parent_pos] =
          makeParent(static_merkle_tree[i], static_merkle_tree[i + 1]);
      ++parent_pos;
    }

    return static_merkle_tree;
  }

private:
  std::vector<uint8_t> makeParent(const std::vector<uint8_t> &left,
                                  const std::vector<uint8_t> &right) {
    std::unique_ptr<Botan::HashFunction> hash_function(
        Botan::HashFunction::create("SHA-256"));
    hash_function->update(left);
    hash_function->update(right);

    std::vector<uint8_t> ret_hash(32, 0);
    hash_function->final(ret_hash.data());
    return ret_hash;
  }
};

} // namespace gruut