#ifndef GRUUT_ENTERPRISE_MERGER_TYPES_HPP
#define GRUUT_ENTERPRISE_MERGER_TYPES_HPP

#include "nlohmann/json.hpp"

#include <botan-2/botan/secmem.h>

#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace gruut {

enum class BpStatus {
  IN_BOOT_WAIT,
  IDLE,
  PRIMARY,
  SECONDARY,
  ERROR_ON_SIGNERS,
  UNKNOWN
};

enum class TransactionType { DIGESTS, CERTIFICATES, IMMORTALSMS, UNKNOWN };

const std::string TXTYPE_CERTIFICATES = "CERTIFICATES";
const std::string TXTYPE_DIGESTS = "DIGESTS";
const std::string TXTYPE_IMMORTALSMS = "IMMORTALSMS";

const std::map<TransactionType, std::string> TX_TYPE_TO_STRING = {
    {TransactionType::DIGESTS, TXTYPE_DIGESTS},
    {TransactionType::CERTIFICATES, TXTYPE_CERTIFICATES},
    {TransactionType::IMMORTALSMS, TXTYPE_IMMORTALSMS},
    {TransactionType::UNKNOWN, "UNKNOWN"}};

enum class MessageType : uint8_t {
  MSG_NULL = 0x00,
  MSG_UP = 0x30,
  MSG_PING = 0x31,
  MSG_REQ_BLOCK = 0x32,
  MSG_WELCOME = 0x35,
  MSG_REQ_STATUS = 0x33,
  MSG_RES_STATUS = 0x34,
  MSG_JOIN = 0x54,
  MSG_CHALLENGE = 0x55,
  MSG_RESPONSE_1 = 0x56,
  MSG_RESPONSE_2 = 0x57,
  MSG_SUCCESS = 0x58,
  MSG_ACCEPT = 0x59,
  MSG_LEAVE = 0x5B,
  MSG_TX = 0xB1,
  MSG_REQ_SSIG = 0xB2,
  MSG_SSIG = 0xB3,
  MSG_BLOCK = 0xB4,
  MSG_HEADER = 0xB5,
  // TODO: MSG_CHAIN_INFO 번호는 변경 될 수 있습니다.
  MSG_CHAIN_INFO = 0xB7,
  MSG_ERROR = 0xFF,
  MSG_REQ_CHECK = 0xC0,
  MSG_RES_CHECK = 0xC1
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

enum class ErrorMsgType : int {
  UNKNOWN = 0,
  MERGER_BOOTSTRAP = 11,
  ECDH_ILLEGAL_ACCESS = 21,
  ECDH_MAX_SIGNER_POOL = 22,
  ECDH_TIMEOUT = 23,
  ECDH_INVALID_SIG = 24,
  ECDH_INVALID_PK = 25,
  TIME_SYNC = 61,
  BSYNC_NO_BLOCK = 88,
  NO_SUCH_BLOCK = 89
};

enum class CompressionAlgorithmType : uint8_t {
  LZ4 = 0x04,
  MessagePack = 0x05,
  CBOR = 0x06,
  NONE = 0xFF
};

enum class SignerStatus { UNKNOWN, TEMPORARY, ERROR, GOOD };

// DB
enum class DBType : int {
  BLOCK_HEADER,
  BLOCK_HEIGHT,
  BLOCK_RAW,
  BLOCK_LATEST,
  TRANSACTION,
  LEDGER,
  BLOCK_BACKUP
};

enum class BlockState { RECEIVED, TOSAVE, TODELETE, RETRIED, RESERVED };

enum class ExitCode {
  NORMAL,
  ERROR_ABORT,
  ERROR_COMMON,
  ERROR_SYNC_ALONE,
  ERROR_SYNC_FAIL,
  ERROR_SKIP_STAGE,
  BLOCK_HEALTH_SKIP,
  ERROR_BLOCK_HEALTH_SKIP
};

using json = nlohmann::json;
using string = std::string;
using bytes = std::vector<uint8_t>;

using hash_t = std::vector<uint8_t>;
using timestamp_t = uint64_t;

constexpr auto TRANSACTION_ID_TYPE_SIZE = 32;
constexpr auto CHAIN_ID_TYPE_SIZE = 8;
using tx_id_type = std::array<uint8_t, TRANSACTION_ID_TYPE_SIZE>;
using localchain_id_type = std::array<uint8_t, CHAIN_ID_TYPE_SIZE>;

using block_height_type = size_t;
using block_header_hash_type = hash_t;
using block_id_type = hash_t;
using block_version_type = uint32_t;
using transaction_root_type = hash_t;
using signature_type = bytes;
using header_length_type = uint32_t;
using content_type = std::string;
using hmac_key_type = Botan::secure_vector<uint8_t>;

using block_layer_t = std::vector<std::string>;

using proof_type = struct _proof_type {
  std::string block_id_b64;
  std::vector<std::pair<bool, std::string>> siblings;
};

using storage_block_type = struct _storage_block_type {
  block_id_type id;
  hash_t hash;
  block_id_type prev_id;
  hash_t prev_hash;
  timestamp_t time;
  block_height_type height;
  bytes block_raw;
  json txs;
};

using nth_link_type = struct _nth_link_type {
  block_id_type id;
  block_id_type prev_id;
  hash_t hash;
  hash_t prev_hash;
  block_height_type height;
  timestamp_t time;
};

using unblk_push_result_type = struct _unblk_push_result_type {
  block_height_type height;
  bool linked;
  block_layer_t block_layer;
};

// All of the blows are the same type. Use them according to the context.
// If you cannot distinguish it, just use id_type
using requestor_id_type = bytes;
using merger_id_type = bytes;
using signer_id_type = bytes;
using servend_id_type = bytes;
using id_type = bytes;

// Message
using message_version_type = uint8_t;

} // namespace gruut
#endif
