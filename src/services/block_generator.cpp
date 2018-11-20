#include "block_generator.hpp"
#include "../chain/merkle_tree.hpp"

namespace gruut {
    PartialBlock BlockGenerator::generatePartialBlock(sha256 transaction_root_id) {
        PartialBlock block;

        auto sent_time = to_string(std::time(0));
        block.sent_time = sent_time;
        // TODO: sender_id, Merger ID 임시로 sent_time으로
        block.sender_id = sent_time;
        // TODO: 위와 같은 이유로 sent_time
        block.chain_id = sent_time;
        // TODO: 위와 같은 이유로 sent_time
        block.height = sent_time;

        block.transaction_root = transaction_root_id;

        return block;
    }
}

