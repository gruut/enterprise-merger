#include "grpc_util.hpp"
#include "../../../include/json_schema.hpp"
#include "msg_schema.hpp"
#include <cstring>
namespace gruut {
std::string
HeaderController::attachHeader(std::string &compressed_json,
                               MessageType msg_type,
                               MACAlgorithmType mac_algo_type,
                               CompressionAlgorithmType compression_algo_type) {
  std::string header;
  uint32_t total_length =
      static_cast<uint32_t>(HEADER_LENGTH + compressed_json.size());

  header.resize(32);
  header[0] = G;
  header[1] = VERSION;
  header[2] = static_cast<uint8_t>(msg_type);
  header[3] = static_cast<uint8_t>(mac_algo_type);
  header[4] = static_cast<uint8_t>(compression_algo_type);
  header[5] = NOT_USED;
  for (int i = 9; i > 6; i--) {
    header[i] |= total_length;
    total_length = (total_length >> 8);
  }
  header[6] |= total_length;

  memcpy(&header[10], &LOCAL_CHAIN_ID[0], 8);
  memcpy(&header[18], &SENDER_ID[0], 8);
  memcpy(&header[26], &RESERVED[0], 6);

  return header + compressed_json;
}
std::string HeaderController::detachHeader(std::string &raw_data) {
  size_t json_length = raw_data.size() - HEADER_LENGTH;
  std::string json_dump(&raw_data[HEADER_LENGTH],
                        &raw_data[HEADER_LENGTH] + json_length);

  return json_dump;
}
bool HeaderController::validateMessage(MessageHeader &msg_header) {
  // TODO: 메세지 검증할때 사용하는 값들은 변경될 수 있습니다.
  return (msg_header.identifier == G && msg_header.version == VERSION &&
          msg_header.mac_algo_type == static_cast<MACAlgorithmType>(MAC));
}
int HeaderController::getJsonSize(MessageHeader &msg_header) {
  int json_size = 0;
  for (int i = 3; i > 0; i--) {
    json_size |= msg_header.total_length[i];
    json_size = (json_size << 8);
  }
  json_size |= msg_header.total_length[0];
  json_size -= HEADER_LENGTH;
  return json_size;
}

MessageHeader HeaderController::parseHeader(std::string &raw_data) {
  MessageHeader msg_header;
  msg_header.identifier = G;
  msg_header.version = VERSION;
  msg_header.message_type = static_cast<MessageType>(raw_data[2]);
  msg_header.mac_algo_type = static_cast<MACAlgorithmType>(raw_data[3]);
  msg_header.compression_algo_type =
      static_cast<CompressionAlgorithmType>(raw_data[4]);
  msg_header.dummy = static_cast<uint8_t>(raw_data[5]);
  memcpy(&msg_header.total_length[0], &raw_data[6], 4);
  memcpy(&msg_header.local_chain_id[0], &raw_data[10], 8);
  memcpy(&msg_header.sender_id[0], &raw_data[18], 8);
  memcpy(&msg_header.reserved_space[0], &raw_data[26], 6);

  return msg_header;
}

bool JsonValidator::validateSchema(json json_object, MessageType msg_type) {
  using nlohmann::json;
  using nlohmann::json_schema_draft4::json_validator;

  json_validator schema_validator;
  schema_validator.set_root_schema(MessageSchema::getSchema(msg_type));

  try {
    schema_validator.validate(json_object);
    std::cout << "Validation succeeded" << std::endl;
    return true;
  } catch (const std::exception &e) {
    std::cout << "Validation failed : " << e.what() << std::endl;
    return false;
  }
}
} // namespace gruut
