#include "../chain/types.hpp"
#include "block_generator.hpp"
#include "../chain/merkle_tree.hpp"

namespace gruut {
    PartialBlock BlockGenerator::generatePartialBlock(Transactions transactions) {
        PartialBlock block;

        auto sent_time = to_string(std::time(0));
        block.sent_time = sent_time;
        // TODO: sender_id, Merger ID 임시로 sent_time으로
        block.sender_id = sent_time;
        // TODO: 위와 같은 이유로 sent_time
        block.chain_id = sent_time;
        // TODO: 위와 같은 이유로 sent_time
        block.height = sent_time;

        MerkleTree tree;
        vector<transaction_id_type> transaction_ids_list;
        std::for_each(transactions.begin(), transactions.end(), [&transaction_ids_list](Transaction &transaction) {
            transaction_ids_list.push_back(transaction.transaction_id);
        });

        block.transaction_root = tree.generate(transaction_ids_list);

        return block;
    }
}

