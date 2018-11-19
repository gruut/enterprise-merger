#include "block_generator.hpp"

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


//        auto first_transaction = transactions.front();
        // TODO: transaction의 root는 어떻게 결정?
        block.transaction_root = sent_time;

        return block;
    }
}

