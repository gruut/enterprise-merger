#ifndef GRUUT_ENTERPRISE_MERGER_TYPES_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPES_HPP

#include <string>
#include <vector>

namespace gruut {
    enum class TransactionType {
        CHECKSUM,
        CERTIFICATE
    };

    enum class BlockType {
        PARTIAL,
        NORMAL
    };

    enum class MessageType : uint8_t {
        MSG_JOIN = 0x54,
        MSG_CHALLENGE = 0x55,
        MSG_RESPONSE = 0x56,
        MSG_ACCEPT = 0x57,
        MSG_ECHO = 0x58,
        MSG_LEAVE = 0x59,
        MSG_REQ_SSIG = 0xB2,
        MSG_SSIG = 0xB3,
        MSG_BLOCK = 0xB4,
        MSG_ERROR = 0xFF
    };

    enum class MACAlgorithmType : uint8_t {
        RSA = 0x00,
        ECDSA = 0x01,
        EdDSA = 0x02,
        Schnorr = 0x03,

        HMAC = 0xF1,
        NONE = 0xFF
    };

    enum class CompressionAlgorithmType : uint8_t {
        LZ4 = 0x04,
        NONE = 0xFF
    };

    using sha256 = std::string;
    using timestamp = std::string;
    using block_height_type = std::string;

    using transaction_id_type = sha256;
    using requestor_id_type = sha256;
    // TODO: Trnasaction에서는 256비트임
    using sender_id_type = sha256;
    using transaction_root_type = sha256;
    using chain_id_type = sha256;
    using signature_type = sha256;

    using content_type = std::string;

    // Message
    using local_chain_id_type = uint64_t;
}
#endif
