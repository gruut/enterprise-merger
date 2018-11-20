#include "merkle_tree.hpp"
#include "../utils/sha256.hpp"

namespace gruut {
    sha256 MerkleTree::generate(std::vector<sha256> transaction_ids) {
        if (transaction_ids.empty()) return sha256();

        while (transaction_ids.size() > 1) {
            if (transaction_ids.size() % 2)
                transaction_ids.push_back(transaction_ids.back());

            for (auto i = 0; i < transaction_ids.size() / 2; i++) {
                transaction_ids[i] = make_parent_node(transaction_ids[2 * i], transaction_ids[2 * i + 1]);
            }

            transaction_ids.resize(transaction_ids.size() / 2);
        }

        return transaction_ids.front();
    }

    sha256 MerkleTree::make_parent_node(const sha256 &l, const sha256 &r) {
        const sha256 parentNode = l + r;
        return Sha256::encrypt(parentNode);
    }
}


