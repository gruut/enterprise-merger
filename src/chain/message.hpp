#ifndef GRUUT_HANA_MERGER_MESSAGE_HPP
#define GRUUT_HANA_MERGER_MESSAGE_HPP

#include "../../include/nlohmann/json.hpp"
#include "types.hpp"

namespace gruut {
    struct MessageHeader {
        const uint8_t version = (1 << 6) | (1 << 2) | (1 << 1) | 1;
        MessageType message_type;
        MACAlgorithmType mac_algo_type = MACAlgorithmType::RSA;
        CompressionAlgorithmType compression_algo_type;
        uint8_t dummy;
        uint32_t total_length;
        local_chain_id_type local_chain_id;
        uint64_t sender_id;
        uint8_t reserved_space[6];
        signature_type hmac;
    };

    struct Message : public MessageHeader {
        Message() = delete;

        Message(MessageHeader &header) : MessageHeader(header) {}

        nlohmann::json data;
    };
}
#endif
