/**
 * @file types.hpp
 * @brief 타입의 열거 클래스
 */

#ifndef GRUUT_ENTERPRISE_MERGER_TYPES_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPES_HPP

#include <botan/secmem.h>
#include <string>
#include <vector>

namespace gruut {

/**
 * @brief 트랜잭션 타입 열거 클래스
 */
enum class TransactionType { CHECKSUM, CERTIFICATE };

/**
 * @brief 블록 타입 열거 클래스
 */
enum class BlockType { PARTIAL, NORMAL };

/**
 * @brief 메시지 타입 열거 클래스
 */
enum class MessageType : uint8_t {
  MSG_UP = 0x30,
  MSG_PING = 0x31,
  MSG_REQ_BLOCK = 0x32,
  MSG_JOIN = 0x54,
  MSG_CHALLENGE = 0x55,
  MSG_RESPONSE_FIRST = 0x56,
  MSG_RESPONSE_SECOND = 0x57,
  MSG_SUCCESS = 0x58,
  MSG_ACCEPT = 0x59,
  MSG_ECHO = 0x5A,
  MSG_LEAVE = 0x5B,
  MSG_REQ_SSIG = 0xB2,
  MSG_SSIG = 0xB3,
  MSG_BLOCK = 0xB4,
  MSG_ERROR = 0xFF
};

/**
 * @brief MAC 타입 열거 클래스
 */
enum class MACAlgorithmType : uint8_t {
  RSA = 0x00,
  ECDSA = 0x01,
  EdDSA = 0x02,
  Schnorr = 0x03,

  HMAC = 0xF1,
  NONE = 0xFF
};

/**
 * @brief 압축 타입 열거 클래스
 */
enum class CompressionAlgorithmType : uint8_t { LZ4 = 0x04, NONE = 0xFF };

using sha256 = Botan::secure_vector<uint8_t>;
using timestamp = std::string;
using block_height_type = std::string;

using transaction_id_type = sha256;
using requestor_id_type = sha256;
using sender_id_type = sha256;
using signer_id_type = sender_id_type;
using transaction_root_type = sha256;
using chain_id_type = sha256;
using signature_type = sha256;

using content_type = std::string;

// Message
using local_chain_id_type = uint8_t;
} // namespace gruut
#endif
