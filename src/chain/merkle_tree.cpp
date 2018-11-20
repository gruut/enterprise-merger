#include "merkle_tree.hpp"
#include "../utils/sha256.hpp"

namespace gruut {
    sha256 MerkleTree::generate(std::vector<sha256> transaction_ids) {
        if (transaction_ids.empty()) return sha256();

        while (transaction_ids.size() > 1) {
            if (transaction_ids.size() % 2)
                transaction_ids.push_back(transaction_ids.back());

            for (auto i = 0; i < transaction_ids.size() / 2; i++) {
                transaction_ids[i] = makeParentNode(transaction_ids[2 * i], transaction_ids[2 * i + 1]);
            }

            transaction_ids.resize(transaction_ids.size() / 2);
        }

        return transaction_ids.front();
    }

    sha256 MerkleTree::makeParentNode(const sha256 &l, const sha256 &r) {
        const sha256 parent_node = l + r;
        return Sha256::encrypt(parent_node);
    }
}


