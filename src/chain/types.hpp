#ifndef GRUUT_ENTERPRISE_MERGER_TYPES_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPES_HPP

#include <botan/secmem.h>
#include <string>
#include <vector>

namespace gruut {
enum class TransactionType { DIGESTS, CERTIFICATE };

enum class BlockType { PARTIAL, NORMAL };

enum class MessageType : uint8_t {
  MSG_NULL = 0x00,
  MSG_UP = 0x30,
  MSG_PING = 0x31,
  MSG_REQ_BLOCK = 0x32,
  MSG_JOIN = 0x54,
  MSG_CHALLENGE = 0x55,
  MSG_RESPONSE_1 = 0x56,
  MSG_RESPONSE_2 = 0x57,
  MSG_SUCCESS = 0x58,
  MSG_ACCEPT = 0x59,
  MSG_ECHO = 0x5A,
  MSG_LEAVE = 0x5B,
  MSG_TX = 0xB1,
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
  RSA_PKCS115 = 0x10,

  HMAC = 0xF1,
  NONE = 0xFF
};

enum class CompressionAlgorithmType : uint8_t { LZ4 = 0x04, NONE = 0xFF };

enum class SignerStatus { UNKNOWN, TEMPORARY, ERROR, GOOD };

// DB
enum class DBType : int {
  BLOCK_HEADER,
  BLOCK_HEIGHT,
  BLOCK_RAW,
  BLOCK_LATEST,
  BLOCK_BODY,
  BLOCK_CERT
};

using sha256 = std::vector<uint8_t>;
using bytes = std::vector<uint8_t>;
using timestamp_type = uint64_t;
using block_height_type = uint64_t;

constexpr auto TRANSACTION_ID_TYPE_SIZE = 32;
using transaction_id_type = std::array<uint8_t, TRANSACTION_ID_TYPE_SIZE>;

using requestor_id_type = sha256;
using merger_id_type = uint64_t;
using signer_id_type = uint64_t;
using transaction_root_type = sha256;
using block_header_hash_type = sha256;
using block_id_type = sha256;
using chain_id_type = uint64_t;
using signature_type = bytes;
using version_type = uint32_t;
using header_length_type = uint32_t;

using content_type = std::string;

using hmac_key_type = Botan::secure_vector<uint8_t>;

// Message
using local_chain_id_type = uint8_t;
} // namespace gruut
#endif
