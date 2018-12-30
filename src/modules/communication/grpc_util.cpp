#include "grpc_util.hpp"
#include "../../../include/json-schema.hpp"
#include "../../config/config.hpp"
#include "../../services/setting.hpp"
#include "../../utils/compressor.hpp"
#include "msg_schema.hpp"
#include <botan-2/botan/hex.h>
#include <botan-2/botan/mac.h>
#include <cstring>

#include "easy_logging.hpp"

namespace gruut {
std::string
HeaderController::attachHeader(std::string &compressed_json,
                               MessageType msg_type,
                               CompressionAlgorithmType compression_algo_type) {
  std::string header;
  uint32_t total_length =
      static_cast<uint32_t>(HEADER_LENGTH + compressed_json.size());

  header.resize(config::HEADER_LENGTH);
  header[0] = config::G;
  header[1] = config::VERSION;
  header[2] = static_cast<uint8_t>(msg_type);
  if (msg_type == MessageType::MSG_ACCEPT ||
      msg_type == MessageType::MSG_REQ_SSIG) {
    header[3] = static_cast<uint8_t>(MACAlgorithmType::HMAC);
  } else {
    header[3] = static_cast<uint8_t>(MACAlgorithmType::NONE);
  }
  header[4] = static_cast<uint8_t>(compression_algo_type);
  header[5] = NOT_USED;
  for (int i = 9; i > 6; --i) {
    header[i] |= total_length;
    total_length = (total_length >> 8);
  }
  header[6] |= total_length;

  auto setting = Setting::getInstance();
  id_type sender_id = setting->getMyId();
  local_chain_id_type chain_id = setting->getLocalChainId();

  memcpy(&header[10], &chain_id[0], CHAIN_ID_TYPE_SIZE);

  if (sender_id.size() >= config::SENDER_ID_LENGTH)
    memcpy(&header[10 + CHAIN_ID_TYPE_SIZE], &sender_id[0],
           config::SENDER_ID_LENGTH);

  memcpy(&header[10 + CHAIN_ID_TYPE_SIZE + config::SENDER_ID_LENGTH],
         &config::RESERVED[0], config::RESERVED_LENGTH);

  return header + compressed_json;
}

MessageHeader HeaderController::parseHeader(std::string &raw_data) {
  MessageHeader msg_header;
  msg_header.sender_id.resize(config::SENDER_ID_LENGTH);
  msg_header.identifier = static_cast<uint8_t>(raw_data[0]);
  msg_header.version = static_cast<uint8_t>(raw_data[1]);
  msg_header.message_type = static_cast<MessageType>(raw_data[2]);
  msg_header.mac_algo_type = static_cast<MACAlgorithmType>(raw_data[3]);
  msg_header.compression_algo_type =
      static_cast<CompressionAlgorithmType>(raw_data[4]);
  msg_header.dummy = static_cast<uint8_t>(raw_data[5]);
  memcpy(&msg_header.total_length[0], &raw_data[6], config::MSG_LENGTH_SIZE);
  memcpy(&msg_header.local_chain_id[0], &raw_data[10], CHAIN_ID_TYPE_SIZE);
  memcpy(&msg_header.sender_id[0], &raw_data[10 + CHAIN_ID_TYPE_SIZE],
         config::SENDER_ID_LENGTH);
  memcpy(&msg_header.reserved_space[0],
         &raw_data[10 + CHAIN_ID_TYPE_SIZE + config::SENDER_ID_LENGTH],
         config::RESERVED_LENGTH);

  return msg_header;
}

int HeaderController::convertU8ToU32BE(uint8_t *len_bytes) {
  return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 |
                          len_bytes[2] << 8 | len_bytes[3]);
}

bool JsonValidator::validateSchema(json json_object, MessageType msg_type) {
  using nlohmann::json;
  using nlohmann::json_schema_draft4::json_validator;

  el::Loggers::getLogger("JVAL");

  json_validator schema_validator;
  schema_validator.set_root_schema(MessageSchema::getSchema(msg_type));

  try {
    schema_validator.validate(json_object);
    return true;
  } catch (const std::exception &e) {
    CLOG(ERROR, "JVAL") << "Validation failed (" << (int) msg_type << ", " << e.what() << ")";
    return false;
  }
}
} // namespace gruut